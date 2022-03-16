# Bridge

The bridge serves as an agnostic endpoint for message streams, providing both storage (transport) and listening. The bridge implementation defines
the capabilities. Bridge pooling is preferred via listener interfaces, invoked when messages are made available. Listeners can either register / subscribe for static schemas with the
message type, or for ordered schemas.

---

Code quick start
- Implementation
    - [Library](../../Source/Libraries/Bridge) </br>

---

### Memory Bridge

The memory bridge provides intra-process transport, reducing redundant memory copies. 

### Network Bridge

The network bridge provides cross-process, cross-machine and cross-network transport using standalone ASIO.
In order to provide drop-in discoverability with minimal setup, a three-step process was introduced.

```

                       Network Boundary
                                                                     ┌─────────────┐
                              │                                      │             │
                              │                               ┌──────► Host Server │
                              │                               │      │             │
                              │                               │      └─────────────┘
                              │                               │
                              │                               │
 ┌────────────────┐           │           ┌───────────────┐   │      ┌─────────────┐
 │                │           │           │               │   │      │             │
 │ Remote Client  ├───────────┼───────────► Host Resolver ├───┼──────► Host Server │
 │                │           │           │               │   │      │             │
 └────────────────┘           │           └───────────────┘   │      └─────────────┘
                              │                               │
                              │                               │
                              │                               │      ┌─────────────┐
                              │                               │      │             │
                              │                               └──────► Host Server │
                              │                                      │             │
                              │                                      └─────────────┘

```

The host server represents an application ready to be instrumented, in order to service any number of clients this is implemented as a server.
In order to provide efficient discoverability of all host servers, the host resolver acts as an on-demand service for server allocation and remote pooling. 
The remote client queries all available servers via the host resolver, handshakes with the host servers are proxied through the host resolver, 
the final host server connection is dedicated.

Cross-network usages is limited by the bandwidth of the network, cross-process usages will bounce off the software tcp stack.
