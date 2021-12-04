// Catch2
#include <catch2/catch.hpp>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStream.h>

// Layer
#include <Loader.h>

TEST_CASE_METHOD(Loader, "Layer.StartupAndShutdown", "[Vulkan]") {
    REQUIRE(AddInstanceLayer("VK_GPUOpen_GBV"));

    // Create the instance & device
    CreateInstance();
    CreateDevice();
}
