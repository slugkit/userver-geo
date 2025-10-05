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

- **MaxmindDbLookup** (`geo/include/slugkit/geo/maxmind_db_lookup.hpp`, `geo/src/slugkit/geo/maxmind_db_lookup.cpp`) - Concrete implementation using MaxMind databases
  - Component name: `"maxmind-db-lookup"`
  - Uses FastPimpl pattern to hide MaxMind implementation details
  - Memory-mapped database access via `MMDB_MODE_MMAP`
  - Configuration requires `database-dir` and `database-file` paths

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

When using MaxmindDbLookup in a userver service, configure it in YAML:

```yaml
components:
  maxmind-db-lookup:
    database-dir: /path/to/databases
    database-file: GeoLite2-City.mmdb
```

The component will:
1. Open the MaxMind database file on initialisation
2. Keep it memory-mapped for fast lookups
3. Close the database on component destruction

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
