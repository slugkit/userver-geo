# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**userver-geo** is a C++ library providing GeoIP resolution components for the userver framework. It wraps the MaxMind libmaxminddb library to provide IP address geolocation lookups as a userver component.

This is a **library subproject** within the larger SlugKit repository (`libs/userver-geo/`). It's designed to be included in larger projects that already have userver configured.

## Architecture

### Core Components

- **LookupComponentBase** (`geo/include/slugkit/geo/lookup_component_base.hpp`) - Abstract base class defining the GeoIP lookup interface
  - Returns `LookupResult` with country code/name, city name, time zone, and coordinates
  - Pure virtual `Lookup(const std::string& ip)` method
  - TODO: Add lookup deadline support

- **MaxmindDbLookup** (`geo/include/slugkit/geo/maxmind_db_lookup.hpp`, `geo/src/slugkit/geo/maxmind_db_lookup.cpp`) - Concrete implementation using MaxMind databases
  - Component name: `"maxmind-db-lookup"`
  - Uses FastPimpl pattern to hide MaxMind implementation details
  - Memory-mapped database access via `MMDB_MODE_MMAP`
  - Configuration requires `database-dir` and `database-file` paths

### Middleware Components

- **GeoMiddlewareConfig** (`geo/include/slugkit/geo/context_config.hpp`) - Configuration component for context variable names
  - Component name: `"geoip-middleware-config"`
  - Configures the names of request context variables that will be set by the middleware
  - Default variable names: `lookup_result`, `country_code`, `country_name`, `city_name`, `time_zone`, `coordinates`

- **GeoMiddlewareFactory** (`geo/include/slugkit/geo/middleware.hpp`, `geo/src/slugkit/geo/middleware.cpp`) - HTTP middleware factory for automatic GeoIP resolution
  - Component name: `"geoip-middleware"`
  - Extracts IP from request header (default: `x-real-ip`, configurable)
  - Supports multiple resolver components with fallback chain (tries resolvers in order until one succeeds)
  - Sets geo information to request context variables for use in handlers
  - Reference to shared `GeoMiddlewareConfig` for context variable naming

### Dependencies

- **userver::core** - Component framework and utilities
- **libmaxminddb** - MaxMind database reader (fetched via CMake FetchContent from GitHub)
- Expects `/userver/cmake` to be in CMAKE_PREFIX_PATH (configured in parent project)

## Build System

### CMake Structure

**Root CMakeLists.txt:**
- Sets Clang as compiler (`/usr/bin/clang++`)
- Requires userver with `universal` and `core` components
- Expects to be included in a larger project with userver pre-configured
- Includes userver module helpers from `/userver/cmake/`

**geo/CMakeLists.txt:**
- Fetches libmaxminddb from GitHub (main branch)
- Builds static library `slugkit-geo`
- Links against `userver::core` and `maxminddb`

### Building

This library is **not standalone** - it must be built as part of a parent project that has userver configured. The parent project's build system (e.g., `make build-in-docker` from the root SlugKit repository) handles building this library.

## Component Configuration

### Basic Lookup Component

Configure MaxmindDbLookup in YAML:

```yaml
components:
  maxmind-db-lookup:
    database-dir: /path/to/databases
    database-file: GeoLite2-City.mmdb
```

### Middleware Setup

To use automatic GeoIP resolution via middleware:

```yaml
components:
  # Optional: Configure context variable names (uses defaults if not specified)
  geoip-middleware-config:
    lookup_result_context: geo_lookup     # default: lookup_result
    country_code_context: country_code    # default: country_code
    country_name_context: country_name    # default: country_name
    city_name_context: city_name          # default: city_name
    time_zone_context: time_zone          # default: time_zone
    coordinates_context: coordinates      # default: coordinates

  # Middleware factory
  geoip-middleware:
    config-name: geoip-middleware-config  # optional, defaults to geoip-middleware-config
    ip-header: x-real-ip                  # optional, defaults to x-real-ip
    resolvers:
      - maxmind-db-lookup                 # List of resolver components (tries in order)
      # - some-online-service             # Fallback to online service if MaxMind fails

# Apply middleware to server
server:
  middlewares:
    - geoip-middleware
```

**Middleware behaviour:**
- Extracts IP address from specified header (default: `x-real-ip`)
- Tries each resolver in order until one returns a result
- Sets resolved geo data to request context variables
- Handlers can access the data via `context.GetData<T>(variable_name)`

## Lookup Result Structure

```cpp
struct LookupResult {
    std::string country_code;                // ISO 3166-1 alpha-2 code
    std::string country_name;                // English country name
    std::optional<std::string> city_name;    // English city name
    std::optional<std::string> time_zone;    // Time zone
    std::optional<Coordinates> coordinates;  // Latitude and longitude
};
```

Returns `std::nullopt` if:
- IP address is empty
- IP address lookup fails (getaddrinfo error)
- Database lookup fails (mmdb error)
- No entry found for the IP address
