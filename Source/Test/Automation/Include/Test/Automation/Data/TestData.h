#pragma once

// Common
#include <Common/IComponent.h>

class TestData : public TComponent<TestData> {
public:
    COMPONENT(TestData);

    /// Application identifier filter
    std::string applicationFilter;
};
