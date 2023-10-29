#pragma once

// Backend
#include "DiagnosticMessage.h"

// Common
#include <Common/Assert.h>

template<typename T>
struct DiagnosticView {
    /// Constructor, provides an argument view into a message
    DiagnosticView(const DiagnosticMessage<T>& message) : base(static_cast<const uint8_t*>(message.argumentBase)) {
        end = base + message.argumentSize;
    }

    /// Get an argument
    /// \tparam T type must be exactly the same
    /// \return argument reference
    template<typename T>
    const T& Get() {
        const T* ptr = reinterpret_cast<const T*>(base);
        base += sizeof(T);
        ASSERT(base <= end, "Out of bounds view");
        return *ptr;
    }

private:
    /// Argument bounds
    const uint8_t* base;
    const uint8_t* end;
};
