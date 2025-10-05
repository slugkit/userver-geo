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

**Usage:**
```cpp
auto& lookup = context.FindComponent<slugkit::geo::MaxmindDbLookup>();
auto result = lookup.Lookup("8.8.8.8");
if (result) {
    LOG_INFO() << "Country: " << result->country_name;
}
```

### Extensibility

The `LookupComponentBase` abstract class allows implementing additional lookup strategies:
- Online GeoIP services (IP-API, ipinfo.io, etc.)
- Custom database formats
- Caching layers
- Fallback chains

To implement a new lookup type, inherit from `LookupComponentBase` and implement the `Lookup()` method.
