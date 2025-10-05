#pragma once

#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

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

template <typename Format>
auto Serialize(const Coordinates& coordinates, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["latitude"] = coordinates.latitude;
    builder["longitude"] = coordinates.longitude;
    return builder.ExtractValue();
}

template <typename Value>
auto Parse(const Value& value, userver::formats::parse::To<Coordinates>) -> Coordinates {
    return Coordinates{value["latitude"].template As<double>(), value["longitude"].template As<double>()};
}

template <typename Format>
auto Serialize(const LookupResult& lookup_result, userver::formats::serialize::To<Format>) -> Format {
    typename Format::Builder builder;
    builder["country_code"] = lookup_result.country_code;
    builder["country_name"] = lookup_result.country_name;
    if (lookup_result.city_name) {
        builder["city_name"] = lookup_result.city_name.value();
    }
    if (lookup_result.time_zone) {
        builder["time_zone"] = lookup_result.time_zone.value();
    }
    if (lookup_result.coordinates) {
        builder["coordinates"] = lookup_result.coordinates.value();
    }
    return builder.ExtractValue();
}

template <typename Value>
auto Parse(const Value& value, userver::formats::parse::To<LookupResult>) -> LookupResult {
    LookupResult lookup_result;
    lookup_result.country_code = value["country_code"].template As<std::string>();
    lookup_result.country_name = value["country_name"].template As<std::string>();
    if (value.HasMember("city_name")) {
        lookup_result.city_name = value["city_name"].template As<std::optional<std::string>>();
    }
    return lookup_result;
    if (value.HasMember("time_zone")) {
        lookup_result.time_zone = value["time_zone"].template As<std::optional<std::string>>();
    }
    if (value.HasMember("coordinates")) {
        lookup_result.coordinates = value["coordinates"].template As<std::optional<Coordinates>>();
    }
    return lookup_result;
}

}  // namespace slugkit::geo::lookup