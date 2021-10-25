#include <catch2/catch.hpp>

// Backend
#include <Backend/IFeature.h>

class TestFeature final : public IFeature {
public:
    FeatureHookTable GetHookTable() final {
        FeatureHookTable table{};
        table.drawIndexed = BindDelegate(this, TestFeature::OnDrawIndexed);
        return table;
    }

    void OnDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        testIndexCount = indexCount;
    }

    uint32_t testIndexCount = 0;
};

TEST_CASE("Backend.Feature") {
    TestFeature feature;

    FeatureHookTable table = feature.GetHookTable();

    table.drawIndexed.Invoke(5, 0, 0, 0, 0);
    REQUIRE(feature.testIndexCount == 5);
}
