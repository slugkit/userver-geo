#include <slugkit/geo/lookup/maxmind_db_lookup.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <maxminddb.h>

namespace slugkit::geo::lookup {

struct MaxmindDb::Impl {
    std::string database_file_;
    MMDB_s database_;
    mutable userver::engine::SharedMutex mutex_;

    Impl(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context)
        : database_file_(config["database-dir"].As<std::string>() + "/" + config["database-file"].As<std::string>())
        , database_{} {
        auto status = MMDB_open(database_file_.c_str(), MMDB_MODE_MMAP, &database_);
        if (status != MMDB_SUCCESS) {
            throw std::runtime_error("Failed to open database file: " + database_file_);
        }
    }

    ~Impl() {
        if (database_.file_content) {
            MMDB_close(&database_);
        }
    }

    auto Reload() -> void {
        std::unique_lock lock(mutex_);
        MMDB_s new_database = {};
        auto status = MMDB_open(database_file_.c_str(), MMDB_MODE_MMAP, &new_database);
        if (status != MMDB_SUCCESS) {
            LOG_ERROR() << "Failed to open database file: " << database_file_;
            return;
        }
        std::swap(database_, new_database);
    }

    auto Lookup(const std::string& ip_str) const -> std::optional<LookupResult> {
        if (ip_str.empty()) {
            return std::nullopt;
        }
        std::shared_lock lock(mutex_);
        int gai_error = 0;
        int mmdb_error = 0;
        auto lookup_result = MMDB_lookup_string(&database_, ip_str.c_str(), &gai_error, &mmdb_error);
        if (gai_error != 0) {
            LOG_ERROR() << "Failed to lookup IP address: " << ip_str
                        << " (getaddrinfo error: " << gai_strerror(gai_error) << ")";
            return std::nullopt;
        }
        if (mmdb_error != 0) {
            LOG_ERROR() << "Failed to lookup IP address: " << ip_str << " (mmdb_error: " << MMDB_strerror(mmdb_error)
                        << ")";
            return std::nullopt;
        }
        if (!lookup_result.found_entry) {
            LOG_ERROR() << "Failed to lookup IP address: " << ip_str << " (not found)";
            return std::nullopt;
        }
        LookupResult result;
        MMDB_entry_data_s entry_data;
        MMDB_get_value(&lookup_result.entry, &entry_data, "country", "iso_code", nullptr);
        result.country_code = std::string(entry_data.utf8_string, entry_data.data_size);
        MMDB_get_value(&lookup_result.entry, &entry_data, "country", "names", "en", nullptr);
        result.country_name = std::string(entry_data.utf8_string, entry_data.data_size);
        MMDB_get_value(&lookup_result.entry, &entry_data, "city", "names", "en", nullptr);
        if (entry_data.has_data) {
            result.city_name = std::string(entry_data.utf8_string, entry_data.data_size);
        }
        MMDB_get_value(&lookup_result.entry, &entry_data, "location", "time_zone", nullptr);
        if (entry_data.has_data) {
            result.time_zone = std::string(entry_data.utf8_string, entry_data.data_size);
        }
        return result;
    }
};

MaxmindDb::MaxmindDb(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : ComponentBase(config, context)
    , impl_{config, context} {
}

MaxmindDb::~MaxmindDb() = default;

auto MaxmindDb::Reload() -> void {
    impl_->Reload();
}

auto MaxmindDb::Lookup(const std::string& ip_str) const -> std::optional<LookupResult> {
    return impl_->Lookup(ip_str);
}

auto MaxmindDb::GetStaticConfigSchema() -> userver::yaml_config::Schema {
    return userver::yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: MaxMind database lookup component
additionalProperties: false
properties:
    database-dir:
        type: string
        description: The path to the MaxMind database directory
    database-file:
        type: string
        description: The name of the MaxMind database file
)");
}

}  // namespace slugkit::geo::lookup
