#include <catch2/catch.hpp>

// Bridge
#include <Bridge/Asio/AsioRemoteClient.h>
#include <Bridge/Asio/AsioHostResolverServer.h>
#include <Bridge/Asio/AsioHostServer.h>
#include <Bridge/Asio/AsioAsyncRunner.h>

// Std
#include <iostream>

TEST_CASE("Bridge.Asio") {
    AsioConfig config;

    // Set up resolver
    AsioHostResolverServer resolver(config);

    std::atomic<bool> allocatedFlag{false};
    resolver.onAllocated.Add(0, [&](const AsioHostClientInfo& info) {
        REQUIRE(std::string("Application.exe") == info.processName);
        REQUIRE(std::string("SampleApplication") == info.applicationName);
        REQUIRE(info.processId == 256);

        allocatedFlag = true;
    });

    AsioHostClientInfo serverInfo;
    serverInfo.processId = 256;
    strcpy_s(serverInfo.processName, "Application.exe");
    strcpy_s(serverInfo.applicationName, "SampleApplication");

    AsioHostServer server(config, serverInfo);

    REQUIRE(server.IsOpen());

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(allocatedFlag.load());

    AsioRemoteConfig remoteConfig;
    remoteConfig.ipvxAddress = kAsioLocalhost;

    AsioRemoteClient remoteClient(remoteConfig);

    AsioRemoteServerResolverDiscoveryRequest::Entry clientEntry;

    std::atomic<bool> discoveryFlag{false};
    std::atomic<bool> connectedFlag{false};

    remoteClient.onDiscovery.Add(0, [&](const AsioRemoteServerResolverDiscoveryRequest::Response& response) {
        REQUIRE(response.entryCount > 0);
        clientEntry = response.entries[0];

        REQUIRE(std::string("Application.exe") == clientEntry.info.processName);
        REQUIRE(std::string("SampleApplication") == clientEntry.info.applicationName);
        REQUIRE(clientEntry.info.processId == 256);

        discoveryFlag = true;
    });

    remoteClient.onConnected.Add(0, [&]() {
        connectedFlag = true;
    });

    remoteClient.DiscoverAsync();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(discoveryFlag.load());

    remoteClient.RequestClientAsync(clientEntry.token);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(connectedFlag.load());
}
