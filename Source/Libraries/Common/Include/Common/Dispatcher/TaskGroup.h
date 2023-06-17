// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

// Common
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Dispatcher/DispatcherBucket.h>

// Std
#include <atomic>

/// User functor
using TaskGroupFunctor = Delegate<void(DispatcherBucket* bucket, void* userData)>;

class TaskGroup {
public:
    TaskGroup(Dispatcher* dispatcher) {
        controller = new (dispatcher->allocators) Controller(dispatcher);
    }

    ~TaskGroup() {
        controller->Detach();
    }

    /// No copy or move
    TaskGroup(const TaskGroup& other) = delete;
    TaskGroup(TaskGroup&& other) = delete;

    /// No copy or move assignment
    TaskGroup& operator=(const TaskGroup& other) = delete;
    TaskGroup& operator=(TaskGroup&& other) = delete;

    /// Chain a task
    /// \param delegate the task delegate
    /// \param userData optional, the user data for the task
    void Chain(const TaskGroupFunctor& delegate, void* userData) {
        controller->AddLink(delegate, userData);
    }

    /// Commit all tasks
    void Commit() {
        controller->Commit();
    }

    /// Get the current bucket, must be disposed of before the last chain finishes
    DispatcherBucket* GetBucket() {
        return &controller->bucket;
    }

private:
    struct Controller {
        struct LinkData {
            TaskGroupFunctor functor;
            void* userData{nullptr};
            DispatcherBucket* bucket{ nullptr };
        };

        Controller(Dispatcher* dispatcher) : dispatcher(dispatcher) {
            bucket.userData = nullptr;
            bucket.completionFunctor = BindDelegate(this, Controller::OnLinkCompleted);
        }

        /// Invoked once a link has been completed
        void OnLinkCompleted(void*) {
            // Out of jobs?
            mutex.lock();
            if (jobs.empty()) {
                mutex.unlock();

                // Release the controller
                Release();
                return;
            }

            // Pop the job
            DispatcherJob job = jobs.front();
            jobs.pop_front();

            // Submit!
            dispatcher->Add(job);
            mutex.unlock();
        }

        /// Link entry point
        void LinkEntry(void* data) {
            auto* linkData = static_cast<LinkData*>(data);
            linkData->functor.Invoke(linkData->bucket, linkData->userData);
            destroy(linkData, dispatcher->allocators);
        }

        /// Add a link to the controller
        /// \param delegate the task delegate
        /// \param userData optional, the user data for the task
        void AddLink(const TaskGroupFunctor& delegate, void* userData) {
            std::lock_guard lock(mutex);

            auto linkData = new (dispatcher->allocators) LinkData;
            linkData->functor = delegate;
            linkData->userData = userData;
            linkData->bucket = &bucket;

            jobs.push_back(DispatcherJob {
                .userData = linkData,
                .delegate = BindDelegate(this, Controller::LinkEntry),
                .bucket = &bucket
            });

            anyLinks = true;
        }

        /// Commit all jobs
        void Commit() {
            // Ignore if empty
            if (jobs.empty()) {
                return;
            }

            /// Pop the first job
            DispatcherJob job = jobs.front();
            jobs.pop_front();

            // Submit!
            dispatcher->Add(job);
        }

        /// Detach from the TaskGroup
        void Detach() {
            // If no links have been submitted, release the controller
            if (!anyLinks) {
                Release();
            }
        }

        /// Destruct this controller
        void Release() {
            destroy(this, dispatcher->allocators);
        }

        std::mutex mutex;
        Dispatcher* dispatcher{nullptr};
        std::list<DispatcherJob> jobs;
        bool anyLinks{false};
        DispatcherBucket bucket;
    };

    Controller* controller{nullptr};
};