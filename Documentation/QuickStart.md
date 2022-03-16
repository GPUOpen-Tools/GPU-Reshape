# Quick Start

General instructions for front end use.

### Discovery Standalone

**[Temporary command line tool]**

All remote instrumentation of applications is currently performed through the `Services.Discovery.Standalone` application. 
First time use of the toolset ensures the appropriate hooking services are initialized, after which the application can be launched after the 
target application to be instrumented has started.

Once started, input the command `discovery` to list all available targets for connection. Targets are applications using an API that can be instrumented.
```
>> discovery
Discovery:
 'UE4Editor.exe' {7A3008AA-739F-4CF2-B621-5C33720D2934}
 'vkcube.exe' {DE6B78F9-86B8-483C-9934-867D747D3E7A}
 ```

In order to connect to a given client, input the client command with the associated guid `client [GUID]`.
```
>> client {7A3008AA-739F-4CF2-B621-5C33720D2934}
Client connected
```

Currently the standalone tool only supports `global` instrumentation.
```
>> global
[Vulkan] Committing 177 shaders and 121 pipelines for instrumentation
[Vulkan] Instrumented 177 shaders and 121 pipelines (1213 ms)
```

Any instrumentation message produced will be printed in the console window.
