#pragma once

#include <slugkit/geo/context_config.hpp>
#include <slugkit/geo/lookup_component_base.hpp>

#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/utils/fast_pimpl.hpp>

namespace slugkit::geo {

/// @brief Factory for geo middleware.
/// Middleware that sets geo information to request context variables based on the IP address.
/// Will set the following context variables:
/// - lookup_result: optional LookupResult, contains all the geo information. but to avoid including the header,
///         individual variables are also set.
/// - country_code: string
/// - country_name: string
/// - city_name: optional string
/// - time_zone: optional string
/// - coordinates: optional string
/// Variable names are configurable.

class GeoMiddlewareFactory : public userver::server::middlewares::HttpMiddlewareFactoryBase {
public:
    static constexpr std::string_view kName = "geoip-middleware";

    GeoMiddlewareFactory(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );
    ~GeoMiddlewareFactory() override;

    [[nodiscard]] auto Create(
        const userver::server::handlers::HttpHandlerBase& handler,
        userver::yaml_config::YamlConfig middleware_config
    ) const -> std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase> override;

    static auto GetStaticConfigSchema() -> userver::yaml_config::Schema;

private:
    constexpr static auto kImplSize = 96UL;
    constexpr static auto kImplAlign = 8UL;
    struct Impl;
    userver::utils::FastPimpl<Impl, kImplSize, kImplAlign> impl_;
};

}  // namespace slugkit::geo