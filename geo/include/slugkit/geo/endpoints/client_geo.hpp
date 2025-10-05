#pragma once

#include <slugkit/geo/context_config.hpp>

#include <userver/server/handlers/http_handler_json_base.hpp>

namespace slugkit::geo::endpoints {

/// @brief Handler for debugging client geo information
/// Uses context set by geoip-middleware
/// Not really for production use
class ClientGeoHandler : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-client-geo";

    ClientGeoHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    auto HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& request_json,
        userver::server::request::RequestContext& context
    ) const -> userver::formats::json::Value override;

private:
    const GeoMiddlewareConfig& context_config_;
};

}  // namespace slugkit::geo::endpoints