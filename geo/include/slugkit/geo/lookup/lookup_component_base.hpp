#pragma once

#include <slugkit/geo/lookup/result.hpp>

#include <userver/components/component_base.hpp>

namespace slugkit::geo::lookup {

class ComponentBase : public userver::components::ComponentBase {
public:
    ComponentBase(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    )
        : userver::components::ComponentBase(config, context) {
    }

    // TODO Add lookup deadline
    [[nodiscard]] virtual auto Lookup(const std::string& ip) const -> std::optional<LookupResult> = 0;
};

}  // namespace slugkit::geo::lookup