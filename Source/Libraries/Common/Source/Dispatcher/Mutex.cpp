#include <Common/Dispatcher/Mutex.h>

Mutex::Mutex() {
    mutex = new std::mutex;
}

Mutex::~Mutex() {
    delete mutex;
}

void Mutex::Lock() {
    mutex->lock();
}

void Mutex::Unlock() {
    mutex->unlock();
}
