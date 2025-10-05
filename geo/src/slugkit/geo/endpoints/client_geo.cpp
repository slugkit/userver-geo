#include <slugkit/geo/endpoints/client_geo.hpp>
#include <slugkit/geo/lookup/result.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

namespace slugkit::geo::endpoints {

ClientGeoHandler::ClientGeoHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : HttpHandlerJsonBase(config, context)
    , context_config_(context.FindComponent<GeoMiddlewareConfig>()) {
}

auto ClientGeoHandler::HandleRequestJsonThrow(
    [[maybe_unused]] const userver::server::http::HttpRequest& request,
    [[maybe_unused]] const userver::formats::json::Value& request_json,
    userver::server::request::RequestContext& context
) const -> userver::formats::json::Value {
    auto lookup_result = context.GetDataOptional<lookup::LookupResult>(context_config_.config.lookup_result_context);
    if (lookup_result) {
        return Serialize(*lookup_result, userver::formats::serialize::To<userver::formats::json::Value>());
    }
    return {};
}

}  // namespace slugkit::geo::endpoints