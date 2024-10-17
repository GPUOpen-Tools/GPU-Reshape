// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

// Automation
#include <Test/Automation/Connection/MessageTask.h>

// Forward declarations
class PoolingListener;

template<typename T>
struct PooledMessage {
    /// Constructor
    PooledMessage() = default;

    /// Disallow copy or copy assignment
    PooledMessage(const PooledMessage&) = delete;
    PooledMessage& operator=(const PooledMessage&) = delete;

    /// Construct from pooling
    /// \param pooler external pooler
    /// \param mode pooling storage mechanism
    explicit PooledMessage(PoolingListener* pooler, PoolingMode mode = PoolingMode::StoreAndRelease) : pooler(pooler) {
        task.mode = mode;

        // Register this task with the pooler
        PooledMessage<void>::Register(pooler, GetSchema(), &task);
    }

    /// Deconstructor
    ~PooledMessage() {
        if (!task.IsReleased()) {
            PooledMessage<void>::Deregister(pooler, GetSchema(), &task);
        }
    }

    /// Move constructor
    PooledMessage(PooledMessage&& from) noexcept : pooler(from.pooler) {
        // Transfer ownership of the task
        if (!from.task.IsReleased()) {
            PooledMessage<void>::Transfer(pooler, GetSchema(), &from.task, &task);
        }

        // Transfer all contents
        from.task.SafeTransfer(task);
    }

    /// Move assignment operator
    PooledMessage& operator=(PooledMessage&& from) noexcept {
        pooler = from.pooler;

        // Transfer ownership of the task
        if (!from.task.IsReleased()) {
            PooledMessage<void>::Transfer(pooler, GetSchema(), &from.task, &task);
        }

        // Transfer all contents
        from.task.SafeTransfer(task);
        return *this;
    }

    /// Get the message contents
    ///   ? Waits for the message if nothing has been acquired yet
    const T* Get() {
        task.WaitForFirstAcquire();
        return MessageStreamView<>(task.GetReleasedStream()).GetIterator().Get<T>();
    }

    /// Pool the message contents, does not wait
    /// \param clear if true, clears previous contents
    /// \return nullptr if not acquired
    const T* Pool(bool clear = false) {
        if (!task.Pool(clear)) {
            return nullptr;
        }

        return MessageStreamView<>(task.GetReleasedStream()).GetIterator().Get<T>();
    }

    /// Check if the message has arrived
    bool IsReady() {
        return task.IsAcquired();
    }

    /// Wait for the message
    void Wait() {
        task.WaitForNextAcquire();
    }

    /// Wait for the message
    template<typename R, typename P>
    bool Wait(const std::chrono::duration<R, P>& timeout) {
        return task.WaitForNextAcquire(timeout);
    }

    /// Access the message contents
    const T* operator->() {
        return Get();
    }

    /// Check if the message has arrived
    operator bool() {
        return IsReady();
    }

    /// Get the expecting schema
    MessageSchema GetSchema() const {
        return MessageSchema {
            .type = MessageSchemaType::Ordered,
            .id = T::kID
        };
    }

protected:
    template<typename> friend struct PooledMessage;

    /// Helper methods for (pooler) implementation hiding
    static void Register(PoolingListener* listener, MessageSchema schema, MessageTask* task);
    static void Deregister(PoolingListener* listener, MessageSchema schema, MessageTask* task);
    static void Transfer(PoolingListener* listener, MessageSchema schema, MessageTask* from, MessageTask* to);

private:
    /// Underlying pooler
    PoolingListener* pooler{nullptr};

    /// Wrapped task
    MessageTask task;
};
