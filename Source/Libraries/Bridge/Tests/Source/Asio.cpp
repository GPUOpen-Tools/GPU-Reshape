// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
    REQUIRE(resolver.IsOpen());

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

    AsioRemoteClient remoteClient;
    remoteClient.Connect(remoteConfig);

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

    remoteClient.onConnected.Add(0, [&](const AsioHostResolverClientRequest::ServerResponse& response) {
        connectedFlag = true;
    });

    remoteClient.DiscoverAsync();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(discoveryFlag.load());

    remoteClient.RequestClientAsync(clientEntry.token);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(connectedFlag.load());
}

TEST_CASE("Bridge.AsioAsync") {
    AsioConfig config;

    // Set up resolver
    AsioHostResolverServer resolver(config);
    REQUIRE(resolver.IsOpen());

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

    AsioRemoteClient remoteClient;

    std::atomic<bool> asyncConnectFlag{false};
    
    remoteClient.SetAsyncConnectedCallback([&] {
        asyncConnectFlag = true;
    });

    remoteClient.ConnectAsync(remoteConfig);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(asyncConnectFlag.load());

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

    remoteClient.onConnected.Add(0, [&](const AsioHostResolverClientRequest::ServerResponse& response) {
        connectedFlag = true;
    });

    remoteClient.DiscoverAsync();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(discoveryFlag.load());

    remoteClient.RequestClientAsync(clientEntry.token);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(connectedFlag.load());
}

TEST_CASE("Bridge.AsioAsyncCancel") {
    AsioConfig config;

    // Set up resolver
    AsioHostResolverServer resolver(config);
    REQUIRE(resolver.IsOpen());

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

    AsioRemoteClient remoteClient;

    std::atomic<bool> asyncConnectFlag{false};

    remoteClient.SetAsyncConnectedCallback([&] {
        asyncConnectFlag = true;
    });

    AsioRemoteConfig remoteConfig;
    remoteConfig.ipvxAddress = "127.255.255.0";

    remoteClient.ConnectAsync(remoteConfig);
    remoteClient.Stop();

    remoteClient.ConnectAsync(remoteConfig);
    remoteClient.Cancel();
    
    remoteConfig.ipvxAddress = kAsioLocalhost;

    remoteClient.ConnectAsync(remoteConfig);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(asyncConnectFlag.load());

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

    remoteClient.onConnected.Add(0, [&](const AsioHostResolverClientRequest::ServerResponse& response) {
        connectedFlag = true;
    });

    remoteClient.DiscoverAsync();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(discoveryFlag.load());

    remoteClient.RequestClientAsync(clientEntry.token);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(connectedFlag.load());
}

TEST_CASE("Bridge.LongStop") {
    AsioConfig config;

    // Set up resolver
    AsioHostResolverServer resolver(config);
    REQUIRE(resolver.IsOpen());

    AsioHostServer server(config, AsioHostClientInfo{});

    REQUIRE(server.IsOpen());

    AsioRemoteClient remoteClient;

    std::atomic<bool> asyncConnectFlag{false};

    remoteClient.SetAsyncConnectedCallback([&] {
        asyncConnectFlag = true;
    });

    AsioRemoteConfig remoteConfig;

    // Connect to real host
    remoteConfig.ipvxAddress = kAsioLocalhost;
    remoteClient.ConnectAsync(remoteConfig);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    remoteClient.DiscoverAsync();
    remoteClient.Stop();

    REQUIRE(asyncConnectFlag.load());
    asyncConnectFlag.store(false);

    // Connect to fake host
    remoteConfig.ipvxAddress = "127.0.0.0";
    remoteClient.ConnectAsync(remoteConfig);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    remoteClient.Stop();

    REQUIRE(!asyncConnectFlag.load());

    // Connect to real host
    remoteConfig.ipvxAddress = kAsioLocalhost;
    remoteClient.ConnectAsync(remoteConfig);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    remoteClient.Stop();

    REQUIRE(asyncConnectFlag.load());
    asyncConnectFlag.store(false);
}
