#pragma once

#include "DiagnosticBucket.h"

template<typename T, typename A>
class DiagnosticBucketScope {
public:
    /// Default constructor
    DiagnosticBucketScope() = default;

    /// Constructor
    /// \param bucket bucket to scope for 
    /// \param value argument to prepend
    DiagnosticBucketScope(DiagnosticBucket<T>* bucket, const A& value) : bucket(bucket), value(value) {
        
    }

    /// Add a new message to this scope
    /// \param type message type
    /// \param args all arguments
    template<typename... TX>
    void Add(T type, TX&&... args) const {
        bucket->Add(type, value, std::forward<TX...>(args)...);
    }

private:
    /// Target bucket
    DiagnosticBucket<T>* bucket{nullptr};

    /// Implicit argument
    A value;
};