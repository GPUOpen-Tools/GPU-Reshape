#include <Backends/DX12/SwapChain.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/SwapChainState.h>

ULONG WINAPI HookIDXGISwapChainRelease(IDXGISwapChain* swapChain) {
    auto table = GetTable(swapChain);

    // Pass down callchain
    LONG users = table.bottom->next_Release(table.next);
    if (users) {
        return users;
    }

    // Cleanup
    delete table.state;

    // OK
    return 0;
}
