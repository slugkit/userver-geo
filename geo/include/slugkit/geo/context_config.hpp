#pragma once

#include <userver/components/component_base.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

#include <string>

namespace slugkit::geo {

/// @brief Context config for geo middleware.
/// Configures the context variables that will be set with the geo middleware.
struct ContextConfig {
    std::string lookup_result_context;
    std::string country_code_context;
    std::string country_name_context;
    std::string city_name_context;
    std::string time_zone_context;
    std::string coordinates_context;
};

/// @brief Config for geo middleware.
/// Configures the context variables that will be set with the geo middleware
/// A separate component to be reused by other components.
class GeoMiddlewareConfig : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "geoip-middleware-config";
    GeoMiddlewareConfig(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    static auto GetStaticConfigSchema() -> userver::yaml_config::Schema;

    ContextConfig config;
};

template <typename Format>
auto Serialize(const ContextConfig& config, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["lookup_result_context"] = config.lookup_result_context;
    builder["country_code_context"] = config.country_code_context;
    builder["country_name_context"] = config.country_name_context;
    builder["city_name_context"] = config.city_name_context;
    builder["time_zone_context"] = config.time_zone_context;
    builder["coordinates_context"] = config.coordinates_context;
    return builder.ExtractValue();
}

template <typename Value>
auto Parse(const Value& value, userver::formats::parse::To<ContextConfig>) -> ContextConfig {
    return ContextConfig{
        value["lookup_result_context"].template As<std::string>("lookup_result"),
        value["country_code_context"].template As<std::string>("country_code"),
        value["country_name_context"].template As<std::string>("country_name"),
        value["city_name_context"].template As<std::string>("city_name"),
        value["time_zone_context"].template As<std::string>("time_zone"),
        value["coordinates_context"].template As<std::string>("coordinates")
    };
}

}  // namespace slugkit::geo
