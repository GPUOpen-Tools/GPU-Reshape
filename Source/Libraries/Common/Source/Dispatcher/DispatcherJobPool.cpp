#include <Common/Dispatcher/DispatcherJobPool.h>

bool DispatcherJobPool::PopBlocking(DispatcherJob &out) {
    // Wait for item or abort signal
    std::unique_lock lock(mutex.Get());
    var.Get().wait(lock, [this] {
        return !pool.empty() || abortFlag;
    });

    // Abort?
    if (abortFlag) {
        return false;
    }

    // Pop back
    out = pool.back();
    pool.pop_back();
    return true;
}
