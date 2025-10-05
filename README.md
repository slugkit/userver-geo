# userver-geo

GeoIP resolution components for userver

## Overview

This library provides GeoIP lookup functionality as userver components. It includes an abstract base class for implementing various lookup strategies and a concrete implementation using MaxMind databases.

## Implementations

### MaxMind Database Lookup

The `MaxmindDbLookup` component provides offline GeoIP lookups using MaxMind databases (GeoLite2 or GeoIP2).

**Features:**
- Memory-mapped database access for fast lookups
- Returns country code/name, city name, time zone, and coordinates
- Component-based lifecycle management

**Configuration:**
```yaml
components:
  maxmind-db-lookup:
    database-dir: /path/to/databases
    database-file: GeoLite2-City.mmdb
```

**Direct usage:**
```cpp
auto& lookup = context.FindComponent<slugkit::geo::MaxmindDbLookup>();
auto result = lookup.Lookup("8.8.8.8");
if (result) {
    LOG_INFO() << "Country: " << result->country_name;
}
```

## Middleware

The library provides HTTP middleware for automatic GeoIP resolution based on request IP addresses.

### Setup

**1. Configure context variable names (optional):**
```yaml
components:
  geoip-middleware-config:
    country_code_context: country_code
    city_name_context: city_name
    # ... other context variable names
```

**2. Configure middleware factory:**
```yaml
components:
  geoip-middleware:
    config-name: geoip-middleware-config  # optional
    ip-header: x-real-ip                  # optional, default: x-real-ip
    resolvers:
      - maxmind-db-lookup
      # - fallback-resolver  # Optional fallback chain

server:
  middlewares:
    - geoip-middleware
```

### Using Geo Data in Handlers

The middleware automatically sets request context variables:

```cpp
void MyHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    // Access individual fields
    auto country_code = context.GetDataOptional<std::string>("country_code");
    auto city_name = context.GetDataOptional<std::string>("city_name");

    // Or access the full lookup result
    auto lookup_result = context.GetDataOptional<slugkit::geo::LookupResult>("lookup_result");

    if (lookup_result) {
        // Use geo data...
    }
}
```

**Features:**
- Supports multiple resolver components with automatic fallback
- Configurable IP header extraction
- Customisable context variable names
- No handler code changes needed - data available via request context

## Extensibility

The `LookupComponentBase` abstract class allows implementing additional lookup strategies:
- Online GeoIP services (IP-API, ipinfo.io, etc.)
- Custom database formats
- Caching layers
- Fallback chains

To implement a new lookup type, inherit from `LookupComponentBase` and implement the `Lookup()` method. The middleware will automatically support it via the `resolvers` configuration.
