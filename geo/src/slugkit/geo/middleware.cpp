#include <slugkit/geo/middleware.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/utils/ip.hpp>
#include <userver/utils/text.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <variant>

namespace slugkit::geo {

namespace {

constexpr std::string_view kDefaultIpHeader = "x-real-ip";

using TrustedNetwork = std::variant<userver::utils::ip::NetworkV4, userver::utils::ip::NetworkV6>;

auto ExtractRealIp(const std::string& header_value, const std::vector<TrustedNetwork>& trusted_proxies, bool recursive)
    -> std::string {
    if (header_value.empty()) {
        return {};
    }

    // Parse comma-separated IP list
    auto ips = userver::utils::text::Split(header_value, ",");

    // Trim whitespace from each IP
    std::vector<std::string> trimmed_ips;
    trimmed_ips.reserve(ips.size());
    for (const auto& ip : ips) {
        auto trimmed = userver::utils::text::Trim(ip);
        if (!trimmed.empty()) {
            trimmed_ips.emplace_back(trimmed);
        }
    }

    if (trimmed_ips.empty()) {
        return {};
    }

    // If not recursive or no trusted proxies, return the first IP
    if (!recursive || trusted_proxies.empty()) {
        return trimmed_ips.front();
    }

    // Walk backwards, skipping trusted proxies
    for (auto it = trimmed_ips.rbegin(); it != trimmed_ips.rend(); ++it) {
        bool is_trusted = std::any_of(trusted_proxies.begin(), trusted_proxies.end(), [&](const auto& network) {
            return std::visit(
                [&](const auto& net) {
                    using NetworkType = std::decay_t<decltype(net)>;
                    using AddressType = typename NetworkType::AddressType;

                    try {
                        AddressType addr;
                        if constexpr (std::is_same_v<AddressType, userver::utils::ip::AddressV4>) {
                            addr = userver::utils::ip::AddressV4FromString(*it);
                        } else {
                            addr = userver::utils::ip::AddressV6FromString(*it);
                        }
                        return net.ContainsAddress(addr);
                    } catch (const userver::utils::ip::AddressSystemError&) {
                        return false;
                    }
                },
                network
            );
        });

        if (!is_trusted) {
            return *it;
        }
    }

    // All IPs are trusted, return the first one
    return trimmed_ips.front();
}

class GeoMiddleware : public userver::server::middlewares::HttpMiddlewareBase {
public:
    static constexpr std::string_view kName = "geo-middleware";

    GeoMiddleware(
        const GeoMiddlewareConfig& context_config,
        std::vector<lookup::ComponentBase const*> geoip_resolvers,
        std::string ip_header,
        std::vector<TrustedNetwork> trusted_proxies,
        bool recursive
    )
        : context_config_(context_config)
        , resolvers_(std::move(geoip_resolvers))
        , ip_header_(std::move(ip_header))
        , trusted_proxies_(std::move(trusted_proxies))
        , recursive_(recursive) {
    }

    void HandleRequest(userver::server::http::HttpRequest& request, userver::server::request::RequestContext& context)
        const override {
        auto header_value = request.GetHeader(ip_header_);
        auto ip_str = ExtractRealIp(header_value, trusted_proxies_, recursive_);

        if (ip_str.empty()) {
            LOG_WARNING() << "No IP found in header: " << ip_header_;
            Next(request, context);
            return;
        }

        auto lookup_result = LookupIp(ip_str);
        if (lookup_result) {
            SetGeoHeaders(context, *lookup_result);
        }
        Next(request, context);
    }

private:
    auto LookupIp(const std::string& ip_str) const -> std::optional<lookup::LookupResult> {
        for (const auto resolver : resolvers_) {
            auto lookup_result = resolver->Lookup(ip_str);
            if (lookup_result) {
                LOG_INFO() << "Resolved IP: " << ip_str << " to " << lookup_result->country_code;
                return lookup_result;
            }
        }
        LOG_WARNING() << "Failed to resolve IP: " << ip_str;
        return std::nullopt;
    }
    auto SetGeoHeaders(userver::server::request::RequestContext& context, const lookup::LookupResult& lookup_result)
        const -> void {
        context.SetData(context_config_.config.lookup_result_context, lookup_result);
        context.SetData(context_config_.config.country_code_context, lookup_result.country_code);
        context.SetData(context_config_.config.country_name_context, lookup_result.country_name);
        context.SetData(context_config_.config.city_name_context, lookup_result.city_name);
        context.SetData(context_config_.config.time_zone_context, lookup_result.time_zone);
        context.SetData(context_config_.config.coordinates_context, lookup_result.coordinates);
    }

private:
    const GeoMiddlewareConfig& context_config_;
    std::vector<lookup::ComponentBase const*> resolvers_;
    std::string ip_header_;
    std::vector<TrustedNetwork> trusted_proxies_;
    bool recursive_;
};

}  // namespace

struct GeoMiddlewareFactory::Impl {
    const GeoMiddlewareConfig& context_config_;
    std::vector<lookup::ComponentBase const*> resolvers_;
    std::string ip_header_;
    std::vector<TrustedNetwork> trusted_proxies_;
    bool recursive_;

    Impl(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context)
        : context_config_(context.FindComponent<GeoMiddlewareConfig>(
              config["config-name"].As<std::string>("geoip-middleware-config")
          ))
        , ip_header_(config["ip-header"].As<std::string>(kDefaultIpHeader))
        , recursive_(config["recursive"].As<bool>(false)) {
        auto resolver_names = config["resolvers"].As<std::vector<std::string>>();
        for (const auto& resolver_name : resolver_names) {
            resolvers_.push_back(&context.FindComponent<lookup::ComponentBase>(resolver_name));
        }
        if (resolvers_.empty()) {
            throw std::runtime_error("No geoip resolvers provided");
        }

        // Parse trusted proxy networks
        auto trusted_proxy_cidrs = config["trusted-proxies"].As<std::vector<std::string>>({});
        for (const auto& cidr : trusted_proxy_cidrs) {
            try {
                // Try IPv4 first
                auto network_v4 = userver::utils::ip::NetworkV4FromString(cidr);
                trusted_proxies_.push_back(network_v4);
            } catch (const std::exception&) {
                try {
                    // Try IPv6
                    auto network_v6 = userver::utils::ip::NetworkV6FromString(cidr);
                    trusted_proxies_.push_back(network_v6);
                } catch (const std::exception&) {
                    throw std::runtime_error(fmt::format("Invalid CIDR notation: {}", cidr));
                }
            }
        }
    }
};

GeoMiddlewareFactory::GeoMiddlewareFactory(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : userver::server::middlewares::HttpMiddlewareFactoryBase(config, context)
    , impl_{config, context} {
}

GeoMiddlewareFactory::~GeoMiddlewareFactory() = default;

auto GeoMiddlewareFactory::Create(
    const userver::server::handlers::HttpHandlerBase& handler,
    userver::yaml_config::YamlConfig middleware_config
) const -> std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase> {
    return std::make_unique<GeoMiddleware>(
        impl_->context_config_, impl_->resolvers_, impl_->ip_header_, impl_->trusted_proxies_, impl_->recursive_
    );
}

auto GeoMiddlewareFactory::GetStaticConfigSchema() -> userver::yaml_config::Schema {
    return userver::yaml_config::MergeSchemas<userver::server::middlewares::HttpMiddlewareFactoryBase>(R"(
type: object
description: Geo middleware configuration
additionalProperties: false
properties:
    config-name:
        type: string
        description: The name of the geoip middleware config component
        defaultDescription: geoip-middleware-config
    resolvers:
        type: array
        items:
            type: string
            description: The name of the geoip resolver component
        description: |
            The name of the geoip resolver component
            If multiple components are provided, the first one that returns a result will be used.
    ip-header:
        type: string
        description: The name of the header to use for the IP address (e.g., x-real-ip, x-forwarded-for)
        defaultDescription: x-real-ip
    trusted-proxies:
        type: array
        items:
            type: string
            description: Trusted proxy network in CIDR notation (e.g., 10.0.0.0/8, 2001:db8::/32)
        description: |
            List of trusted proxy networks in CIDR notation.
            When recursive is enabled, IPs from these networks are skipped when parsing X-Forwarded-For.
        defaultDescription: empty array
    recursive:
        type: boolean
        description: |
            Enable recursive IP extraction (similar to nginx real_ip_recursive).
            When true, walks backwards through X-Forwarded-For header, skipping trusted proxies.
            When false, uses the first IP from the header.
        defaultDescription: false
)");
}

}  // namespace slugkit::geo