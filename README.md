# userver-geo

GeoIP resolution components for userver

## Overview

This library provides GeoIP lookup functionality as userver components. It includes an abstract base class for implementing various lookup strategies and a concrete implementation using MaxMind databases.

## Implementations

### MaxMind Database Lookup

The `lookup::MaxmindDb` component provides offline GeoIP lookups using MaxMind databases (GeoLite2 or GeoIP2).

**Features:**
- Memory-mapped database access for fast lookups
- Returns country code/name, city name, time zone, and coordinates
- Hot reload support without service restart
- Thread-safe with `engine::SharedMutex` (concurrent reads, exclusive writes)
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
auto& lookup = context.FindComponent<slugkit::geo::lookup::MaxmindDb>();
auto result = lookup.Lookup("8.8.8.8");
if (result) {
    LOG_INFO() << "Country: " << result->country_name;
}
```

**Hot reload:**
The database can be reloaded without restarting the service via the reload endpoint (see Endpoints section below).

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
    config-name: geoip-middleware-config  # optional, default: geoip-middleware-config
    ip-header: x-forwarded-for            # optional, default: x-real-ip
    recursive: true                       # optional, default: false
    trusted-proxies:                      # optional, default: []
      - 10.0.0.0/8
      - 172.16.0.0/12
      - 192.168.0.0/16
      - 2001:db8::/32
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
- **X-Forwarded-For parsing** with trusted proxy support (similar to nginx `real_ip_recursive`)
- **Recursive IP extraction**: Walks backwards through X-Forwarded-For, skipping trusted proxies
- **CIDR notation**: Supports both IPv4 and IPv6 trusted proxy networks
- **Multiple resolver fallback**: Tries resolvers in order until one succeeds
- **Configurable headers**: Extract IP from `x-real-ip`, `x-forwarded-for`, or custom header
- **Customisable context variables**: Configure the names of request context variables
- **Zero handler changes**: Data automatically available via request context

### Nginx Configuration

#### Option 1: Using X-Forwarded-For with Middleware Parsing

Let nginx pass through X-Forwarded-For, and configure the middleware to handle trusted proxies:

```nginx
server {
    location / {
        proxy_pass http://backend;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

Then configure the middleware:
```yaml
geoip-middleware:
  ip-header: x-forwarded-for
  recursive: true
  trusted-proxies:
    - 10.0.0.0/8
    - 172.16.0.0/12
    - 192.168.0.0/16
```

#### Option 2: Let Nginx Handle Real IP

Let nginx resolve the real IP and pass it via X-Real-IP:

```nginx
http {
    # Define trusted proxy addresses
    set_real_ip_from 10.0.0.0/8;
    set_real_ip_from 172.16.0.0/12;
    set_real_ip_from 192.168.0.0/16;

    real_ip_header X-Forwarded-For;
    real_ip_recursive on;

    server {
        location / {
            proxy_pass http://backend;
            proxy_set_header X-Real-IP $remote_addr;
        }
    }
}
```

Then configure the middleware:
```yaml
geoip-middleware:
  ip-header: x-real-ip  # default
```

**Security:** Only add trusted proxy IPs to prevent IP spoofing. Use the same CIDR ranges in both nginx and middleware configurations.

## Endpoints

The library provides HTTP handlers for operations and debugging.

### Reload MaxMind Database

**Handler:** `slugkit::geo::endpoints::ReloadMaxmindDb`

Triggers a hot reload of the MaxMind database without restarting the service.

**Configuration:**
```yaml
components:
  handler-reload-maxmind-db:
    path: /admin/geo/reload
    method: POST
    task_processor: main-task-processor
```

**Usage:**
```bash
curl -X POST http://localhost:8080/admin/geo/reload
```

**Security considerations:**
- Deploy on internal-only listener or separate admin port
- Add authentication/authorization
- Consider rate limiting
- Typically called from cron job after database updates

### Client Geo Information

**Handler:** `slugkit::geo::endpoints::ClientGeoHandler`

Returns GeoIP information for the client based on middleware context (debugging tool).

**Configuration:**
```yaml
components:
  handler-client-geo:
    path: /api/geo/me
    method: GET,POST
    task_processor: main-task-processor

server:
  middlewares:
    - geoip-middleware  # Required
```

**Response example:**
```json
{
  "country_code": "US",
  "country_name": "United States",
  "city_name": "San Francisco",
  "time_zone": "America/Los_Angeles",
  "coordinates": {
    "latitude": 37.7749,
    "longitude": -122.4194
  }
}
```

## Database Management

### Automated Updates with Cron

The library includes scripts for automated database updates:

**1. Configure environment** (`scripts/set_environment.in.sh`):
```bash
export GEOIP_FILES=GeoLite2-City.mmdb:GeoLite2-Country.mmdb:GeoLite2-ASN.mmdb
export GEOIP_DATABASE_DIR=./databases
export GEOIP_UPDATE_TOOL_BIN=./geoipupdate
```

**2. Update script** (`scripts/update_databases.sh`):
- Checks if database files are older than 1 day
- Downloads new databases using `geoipupdate` tool
- Triggers reload endpoint automatically

**3. Cron configuration:**
```cron
# Update GeoIP databases daily at 3 AM
0 3 * * * /path/to/scripts/update_databases.sh
```

**Environment variables:**
- `GEOIP_FILES` - Colon-separated list of database files
- `GEOIP_DATABASE_DIR` - Directory containing databases
- `GEOIP_UPDATE_TOOL_BIN` - Path to geoipupdate binary
- `GEOIP_RELOAD_ENDPOINT` - URL to POST for reload (e.g., `http://localhost:8080/admin/geo/reload`)

### CMake Integration

The `cmake/GeoIp.cmake` module provides functions for downloading and managing GeoIP databases:

```cmake
include(cmake/GeoIp.cmake)

# Download databases during build
download_geoip_databases(
    ${CMAKE_SOURCE_DIR}/GeoIp.conf
    ${CMAKE_BINARY_DIR}/databases
)

# Configure environment script
configure_geoip_environment_script(
    ${CMAKE_SOURCE_DIR}/scripts/set_environment.in.sh
    ${CMAKE_BINARY_DIR}/scripts/set_environment.sh
)
```

The module automatically:
- Downloads the `geoipupdate` tool for your platform
- Downloads GeoIP databases (GeoLite2-Country, GeoLite2-City, GeoLite2-ASN)
- Skips download if databases are less than 1 day old
- Configures environment scripts with correct paths

## Extensibility

The `LookupComponentBase` abstract class allows implementing additional lookup strategies:
- Online GeoIP services (IP-API, ipinfo.io, etc.)
- Custom database formats
- Caching layers
- Fallback chains

To implement a new lookup type, inherit from `LookupComponentBase` and implement the `Lookup()` method. The middleware will automatically support it via the `resolvers` configuration.
