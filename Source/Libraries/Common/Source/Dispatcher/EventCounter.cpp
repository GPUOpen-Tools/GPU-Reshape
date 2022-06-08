#include <Common/Dispatcher/EventCounter.h>

void EventCounter::Wait(uint64_t value) {
    std::unique_lock lock(mutex.Get());
    var.Get().wait(lock, [this, value] { return counter >= value; });
}
