#pragma once

#include <userver/components/component_base.hpp>

#include <optional>
#include <string>

namespace slugkit::geo {

struct Coordinates {
    std::string latitude;
    std::string longitude;
};

struct LookupResult {
    std::string country_code;                // ISO 3166-1 alpha-2 code
    std::string country_name;                // English country name
    std::optional<std::string> city_name;    // English city name
    std::optional<std::string> time_zone;    // Time zone
    std::optional<Coordinates> coordinates;  // Latitude and longitude
};

class LookupComponentBase : public userver::components::ComponentBase {
public:
    LookupComponentBase(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    )
        : userver::components::ComponentBase(config, context) {
    }

    // TODO Add lookup deadline
    [[nodiscard]] virtual auto Lookup(const std::string& ip) const -> std::optional<LookupResult> = 0;
};

}  // namespace slugkit::geo