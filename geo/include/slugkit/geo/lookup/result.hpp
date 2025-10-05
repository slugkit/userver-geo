#pragma once

#include <optional>
#include <string>

namespace slugkit::geo::lookup {

struct Coordinates {
    double latitude;
    double longitude;
};

struct LookupResult {
    std::string country_code;                // ISO 3166-1 alpha-2 code
    std::string country_name;                // English country name
    std::optional<std::string> city_name;    // English city name
    std::optional<std::string> time_zone;    // Time zone
    std::optional<Coordinates> coordinates;  // Latitude and longitude
};

}  // namespace slugkit::geo::lookup