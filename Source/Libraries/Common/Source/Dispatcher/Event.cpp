#include <Common/Dispatcher/Event.h>

void Event::Wait() {
    std::unique_lock lock(mutex.Get());
    var.Get().wait(lock, [this] { return state; });
}
