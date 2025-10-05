#pragma once

#include <slugkit/geo/lookup/maxmind_db_lookup.hpp>

#include <userver/server/handlers/http_handler_base.hpp>

namespace slugkit::geo::endpoints {

class ReloadMaxmindDb : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-reload-maxmind-db";

    ReloadMaxmindDb(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    auto HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const -> std::string override;

private:
    auto Reload() const -> void;

private:
    lookup::MaxmindDb& maxmind_db_lookup_;
};

}  // namespace slugkit::geo::endpoints