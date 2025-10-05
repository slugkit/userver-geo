#include <slugkit/geo/endpoints/reload_maxmind_db.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

namespace slugkit::geo::endpoints {

ReloadMaxmindDb::ReloadMaxmindDb(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : HttpHandlerBase(config, context)
    , maxmind_db_lookup_(context.FindComponent<lookup::MaxmindDb>()) {
}

auto ReloadMaxmindDb::HandleRequestThrow(
    [[maybe_unused]] const userver::server::http::HttpRequest& request,
    [[maybe_unused]] userver::server::request::RequestContext& context
) const -> std::string {
    Reload();
    return "OK";
}

auto ReloadMaxmindDb::Reload() const -> void {
    maxmind_db_lookup_.Reload();
}

}  // namespace slugkit::geo::endpoints