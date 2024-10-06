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
#include <Test/Automation/Connection/PoolingMode.h>
#include <Test/Automation/Connection/MessageController.h>

// Message
#include <Message/MessageStream.h>

class MessageTask {
public:
    /// Has this task been released?
    bool IsReleased() const {
        return acquiredId == UINT64_MAX;
    }

    /// Does this controller have a stream that can be accessed?
    bool HasStream() const {
        return IsReleased() || !snapshot.IsEmpty();
    }

    /// Indicates if this controller should be released on acquisition
    bool ShouldReleaseOnAcquire() const {
        return mode == PoolingMode::StoreAndRelease;
    }

    /// Has this controller been released or previously acquired?
    bool IsAcquired() {
        // If there's any stream, it's at least acquired
        if (HasStream()) {
            return true;
        }

        std::lock_guard lock(controller->mutex);

        // Check the controller commit, unmoved?
        if (controller->commitId == acquiredId) {
            return false;
        }

        // It's been acquired
        OnAcquisition();
        return true;
    }

    /// Wait for the first release or acquisition
    void WaitForFirstAcquire() {
        // If there's already a stream, early out
        if (HasStream()) {
            return;
        }

        std::unique_lock lock(controller->mutex);

        // Try to perform a snapshot before waiting, non-release pooling may benefit from this
        if (Snapshot()) {
            return;
        }

        // No pending data, wait for the next acquisition
        WaitForAcquire(lock);
    }

    /// Wait for the final release or pending acquire
    void WaitForNextAcquire() {
        // If released, early out
        if (IsReleased()) {
            return;
        }

        // Just assume acquire
        std::unique_lock lock(controller->mutex);
        WaitForAcquire(lock);
    }

    /// Wait for the final release or pending acquire
    template<typename R, typename P>
    bool WaitForNextAcquire(const std::chrono::duration<R, P>& timeout) {
        // If released, early out
        if (IsReleased()) {
            return true;
        }

        // Just assume acquire
        std::unique_lock lock(controller->mutex);
        return WaitForAcquire(lock, timeout);
    }

    /// Pool for acquisition without waiting
    /// \param clear if true, clears current (non-release) data before pooling
    /// \return true if acquired
    bool Pool(bool clear = false) {
        // If released, early out
        if (IsReleased()) {
            return true;
        }

        // Clear snapshot data?
        if (clear) {
            snapshot.Clear();
        }

        // Check acquisition states
        std::lock_guard lock(controller->mutex);
        UpdateAcquisition();

        // If there's any stream, pooling has succeeded
        return HasStream();
    }

    /// Safely transfer all contents to another task
    void SafeTransfer(MessageTask* to) {
        // Controllers are optional
        if (controller) {
            controller->mutex.lock();
        }

        // Move all data
        // Note: Swapping is not appropriate, yes, however, the use case is solely on moves.
        //       Which is what everyone says before they forget it.
        to->controller = controller;
        to->acquiredId = acquiredId;
        to->mode = mode;
        to->id = id;
        stream.Swap(to->stream);
        snapshot.Swap(to->snapshot);

        // Cleanup
        if (controller) {
            controller->mutex.unlock();
        }
    }

    /// Get the currently released stream
    ///   ! Must be released, undefined otherwise
    MessageStream& GetReleasedStream() {
        switch (mode) {
            default:
                ASSERT(false, "Invalid mode");
            case PoolingMode::StoreAndRelease:
                return stream;
            case PoolingMode::Replace:
                return snapshot;
        }
    }

    /// Mark this task as released
    void MarkAsReleased() {
        acquiredId = UINT64_MAX;
    }

private:
    /// Perform a snapshot of this task
    bool Snapshot() {
        // If there's no stream data, or the pooling mode is an ill-fit, ignore it
        if (stream.IsEmpty() || mode == PoolingMode::StoreAndRelease) {
            return false;
        }

        // Copy all data
        snapshot.SetData(stream.GetDataBegin(), stream.GetByteSize(), stream.GetCount());
        return true;
    }

    /// Wait for acquisition
    void WaitForAcquire(std::unique_lock<std::mutex>& lock) {
        // Hold until the controller moves ahead of the local commit
        controller->wakeCondition.wait(lock, [&] {
            return controller->commitId > acquiredId;
        });

        // Handle acuisition
        OnAcquisition();
    }

    /// Wait for acquisition
    template<typename R, typename P>
    bool WaitForAcquire(std::unique_lock<std::mutex>& lock, const std::chrono::duration<R, P>& timeout) {
        // Hold until the controller moves ahead of the local commit
        if (!controller->wakeCondition.wait_for(lock, timeout, [&] {
            return controller->commitId > acquiredId;
        })) {
            // Timeout!
            return false;
        }

        // Handle acuisition
        OnAcquisition();
        return true;
    }

    /// Update acquisition states
    void UpdateAcquisition() {
        // Has the controller moved?
        if (controller->commitId > acquiredId) {
            OnAcquisition();
        }
    }

    /// Invoked on actual acquisition
    void OnAcquisition() {
        switch (mode) {
            case PoolingMode::StoreAndRelease:
                MarkAsReleased();
                break;
            case PoolingMode::Replace:
                acquiredId = controller->commitId;
                break;
        }

        // Snapshot the incoming data, if applicable
        Snapshot();
    }

public:
    /// External controller, optional
    MessageController* controller{nullptr};

    /// Local acquisition id
    uint64_t acquiredId{UINT64_MAX};

    /// Storage mechanism
    PoolingMode mode{PoolingMode::StoreAndRelease};

    /// Underlying stream
    MessageStream stream;

    /// Optional snapshot, if the pooling mode requires it
    MessageStream snapshot;

    /// Storage identifier, for slot allocation purposes
    uint64_t id{0};
};
