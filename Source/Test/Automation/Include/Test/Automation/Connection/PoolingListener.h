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

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/Containers/SlotArray.h>
#include <Common/Containers/ObjectPool.h>

// Std
#include <mutex>
#include <map>

class PoolingListener : public IBridgeListener {
public:
    COMPONENT(PoolingListener);

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) override;

    /// Register a new task
    /// \param schema expecting schema for pooling
    /// \param task pooled task
    void Register(MessageSchema schema, MessageTask* task);

    /// Deregister a task
    /// \param schema expecting schema for pooling
    /// \param task pooled task
    void Deregister(MessageSchema schema, MessageTask* task);

    /// Transfer owernship between tasks
    /// \param schema expecting schema for pooling
    /// \param from soruce task
    /// \param to destination task
    void Transfer(MessageSchema schema, MessageTask* from, MessageTask* to);

private:
    struct TaskBucket {
        /// All pending tasks in this bucket
        SlotArray<MessageTask*> tasks;
    };

    /// Commit all pending tasks for a schema
    /// \param schema schema to apply for
    /// \param ptr data begin
    /// \param length byte length of data
    /// \param count number of messages
    void Commit(MessageSchema schema, const void* ptr, size_t length, size_t count);

    /// Commit all pending tasks for a bucket
    /// \param bucket bucket to apply for
    /// \param ptr data begin
    /// \param length byte length of data
    /// \param count number of messages
    void Commit(TaskBucket& bucket, const void* ptr, size_t length, size_t count);

    /// Commit all pending tasks for a task
    /// \param task task to apply for
    /// \param ptr data begin
    /// \param length byte length of data
    /// \param count number of messages
    void Commit(MessageTask* task, const void* ptr, size_t length, size_t count);

private:
    struct SchemaComparator {
        bool operator()(const MessageSchema& lhs, const MessageSchema& rhs) const {
            return std::make_tuple(lhs.type, lhs.id) < std::make_tuple(rhs.type, rhs.id);
        }
    };
    
private:
    /// All pending pooling schemas
    std::map<MessageSchema, TaskBucket, SchemaComparator> poolingSchemas;

    /// Controller pool, managed dynamically
    ObjectPool<MessageController> controllerPool;

    /// Shared lock
    std::mutex mutex;
};
