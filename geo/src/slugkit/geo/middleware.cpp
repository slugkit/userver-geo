#include <slugkit/geo/middleware.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <fmt/format.h>

namespace slugkit::geo {

namespace {

constexpr std::string_view kDefaultIpHeader = "x-real-ip";

class GeoMiddleware : public userver::server::middlewares::HttpMiddlewareBase {
public:
    static constexpr std::string_view kName = "geo-middleware";

    GeoMiddleware(
        const GeoMiddlewareConfig& context_config,
        std::vector<LookupComponentBase const*> geoip_resolvers,
        std::string ip_header
    )
        : context_config_(context_config)
        , geoip_resolvers_(std::move(geoip_resolvers))
        , ip_header_(ip_header) {
    }

    void HandleRequest(userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context)
        const override {
        auto ip_str = request.GetHeader(ip_header_);
        auto lookup_result = LookupIp(ip_str);
        if (lookup_result) {
            LOG_INFO() << "Resolved IP: " << ip_str << " to " << lookup_result->country_code;
            SetGeoHeaders(context, *lookup_result);
        }
        Next(request, context);
    }

private:
    auto LookupIp(const std::string& ip_str) const -> std::optional<LookupResult> {
        for (const auto resolver : geoip_resolvers_) {
            auto lookup_result = resolver->Lookup(ip_str);
            if (lookup_result) {
                return lookup_result;
            }
        }
        return std::nullopt;
    }
    auto SetGeoHeaders(userver::server::request::RequestContext& context, const LookupResult& lookup_result) const
        -> void {
        context.SetData(context_config_.config.lookup_result_context, lookup_result);
        context.SetData(context_config_.config.country_code_context, lookup_result.country_code);
        context.SetData(context_config_.config.country_name_context, lookup_result.country_name);
        context.SetData(context_config_.config.city_name_context, lookup_result.city_name);
        context.SetData(context_config_.config.time_zone_context, lookup_result.time_zone);
        context.SetData(context_config_.config.coordinates_context, lookup_result.coordinates);
    }

private:
    const GeoMiddlewareConfig& context_config_;
    std::vector<LookupComponentBase const*> geoip_resolvers_;
    std::string ip_header_;
};

}  // namespace

GeoMiddlewareFactory::GeoMiddlewareFactory(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : userver::server::middlewares::HttpMiddlewareFactoryBase(config, context)
    , context_config_(
          context.FindComponent<GeoMiddlewareConfig>(config["config-name"].As<std::string>("geoip-middleware-config"))
      )
    , ip_header_(config["ip_header"].As<std::string>(kDefaultIpHeader)) {
    auto resolver_names = config["resolvers"].As<std::vector<std::string>>();
    for (const auto& resolver_name : resolver_names) {
        geoip_resolvers_.push_back(&context.FindComponent<LookupComponentBase>(resolver_name));
    }
    if (geoip_resolvers_.empty()) {
        throw std::runtime_error("No geoip resolvers provided");
    }
}

auto GeoMiddlewareFactory::Create(
    const userver::server::handlers::HttpHandlerBase& handler,
    userver::yaml_config::YamlConfig middleware_config
) const -> std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase> {
    return std::make_unique<GeoMiddleware>(context_config_, geoip_resolvers_, ip_header_);
}

auto GeoMiddlewareFactory::GetStaticConfigSchema() -> userver::yaml_config::Schema {
    return userver::yaml_config::MergeSchemas<userver::server::middlewares::HttpMiddlewareFactoryBase>(R"(
type: object
description: Geo middleware configuration
additionalProperties: false
properties:
    config-name:
        type: string
        description: The name of the geoip middleware config component
        defaultDescription: geoip-middleware-config
    resolvers:
        type: array
        items:
            type: string
            description: The name of the geoip resolver component
        description: |
            The name of the geoip resolver component
            If multiple components are provided, the first one that returns a result will be used.
    ip-header:
        type: string
        description: The name of the header to use for the IP address
        defaultDescription: X-Real-IP
)");
}

}  // namespace slugkit::geo