#pragma once

#include <slugkit/geo/lookup_component_base.hpp>

#include <userver/utils/fast_pimpl.hpp>

namespace slugkit::geo {

class MaxmindDbLookup : public LookupComponentBase {
public:
    static constexpr auto kName = "maxmind-db-lookup";
    MaxmindDbLookup(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );
    ~MaxmindDbLookup() override;

    [[nodiscard]] auto Lookup(const std::string& ip_str) const -> std::optional<LookupResult> override;

    static auto GetStaticConfigSchema() -> userver::yaml_config::Schema;

private:
    constexpr static auto kImplSize = 168UL;
    constexpr static auto kImplAlign = 8UL;
    struct Impl;
    userver::utils::FastPimpl<Impl, kImplSize, kImplAlign> impl_;
};

}  // namespace slugkit::geo