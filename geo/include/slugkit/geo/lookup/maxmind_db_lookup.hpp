#pragma once

#include <slugkit/geo/lookup/lookup_component_base.hpp>

#include <userver/utils/fast_pimpl.hpp>

namespace slugkit::geo::lookup {

class MaxmindDb : public ComponentBase {
public:
    static constexpr auto kName = "maxmind-db-lookup";
    MaxmindDb(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);
    ~MaxmindDb() override;

    auto Reload() -> void;
    [[nodiscard]] auto Lookup(const std::string& ip_str) const -> std::optional<LookupResult> override;

    static auto GetStaticConfigSchema() -> userver::yaml_config::Schema;

private:
    constexpr static auto kImplSize = 496UL;
    constexpr static auto kImplAlign = 8UL;
    struct Impl;
    userver::utils::FastPimpl<Impl, kImplSize, kImplAlign> impl_;
};

}  // namespace slugkit::geo::lookup