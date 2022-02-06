#pragma once

// Common
#include "IComponentTemplate.h"
#include "Registry.h"

/// Component instantiator
/// \tparam T type of the component to instantiate
template<typename T>
class ComponentTemplate final : public IComponentTemplate {
public:
    ComRef<> Instantiate(Registry* registry) {
        return registry->New<T>();
    }
};
