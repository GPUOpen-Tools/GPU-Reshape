#include <Test/Automation/Connection/PooledMessage.h>
#include <Test/Automation/Connection/PoolingListener.h>

template<>
void PooledMessage<void>::Register(PoolingListener* listener, MessageSchema schema, MessageTask* task) {
    listener->Register(schema, task);
}

template<>
void PooledMessage<void>::Deregister(PoolingListener* listener, MessageSchema schema, MessageTask* task) {
    listener->Deregister(schema, task);
}

template<>
void PooledMessage<void>::Transfer(PoolingListener* listener, MessageSchema schema, MessageTask* from, MessageTask* to) {
    listener->Transfer(schema, from, to);
}
