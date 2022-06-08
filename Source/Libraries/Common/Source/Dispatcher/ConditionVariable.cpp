#include <Common/Dispatcher/ConditionVariable.h>

ConditionVariable::ConditionVariable() {
    var = new std::condition_variable;
}

ConditionVariable::~ConditionVariable() {
    delete var;
}

void ConditionVariable::NotifyAll() {
    var->notify_all();
}

void ConditionVariable::NotifyOne() {
    var->notify_one();
}
