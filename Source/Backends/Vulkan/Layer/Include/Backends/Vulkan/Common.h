#pragma once

// Stack allocation
#define ALLOCA(T) static_cast<T*>(alloca(sizeof(T)))
#define ALLOCA_ARRAY(T, N) static_cast<T*>(alloca(sizeof(T) * N))
