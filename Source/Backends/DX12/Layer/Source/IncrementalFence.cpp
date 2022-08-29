#include <Backends/DX12/IncrementalFence.h>

IncrementalFence::~IncrementalFence() {
    if (fence) {
        fence->Release();
    }
}

bool IncrementalFence::Install(ID3D12Device *device, ID3D12CommandQueue *_queue) {
    queue = _queue;

    // Attempt to create fence
    HRESULT hr = device->CreateFence(0x0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr)) {
        return false;
    }

    // OK
    return true;
}

uint64_t IncrementalFence::CommitFence() {
    uint64_t next = ++fenceCPUCommitId;
    queue->Signal(fence, next);
    return next;
}

bool IncrementalFence::IsCommitted(uint64_t head) {
    // Check cached value
    if (fenceGPUCommitId >= head) {
        return true;
    }

    // Cache new head
    fenceGPUCommitId = fence->GetCompletedValue();

    // Ready?
    return (fenceGPUCommitId >= head);
}
