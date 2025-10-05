#include <slugkit/geo/context_config.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace slugkit::geo {

GeoMiddlewareConfig::GeoMiddlewareConfig(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : userver::components::ComponentBase(config, context)
    , config(config.As<ContextConfig>()) {
}

auto GeoMiddlewareConfig::GetStaticConfigSchema() -> userver::yaml_config::Schema {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
type: object
description: Geo middleware configuration
additionalProperties: false
properties:
    lookup_result_context:
        type: string
        description: Lookup result context variable name
    country_code_context:
        type: string
        description: Country code context variable name
    country_name_context:
        type: string
        description: Country name context variable name
    city_name_context:
        type: string
        description: City name context variable name
    time_zone_context:
        type: string
        description: Time zone context variable name
    coordinates_context:
        type: string
        description: Coordinates context variable name
    )");
}

}  // namespace slugkit::geo