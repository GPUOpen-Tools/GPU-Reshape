#pragma once

// Stack allocation
#define Alloca(T) static_cast<T*>(alloca(sizeof(T)))
#define AllocaArray(T, N) static_cast<T*>(alloca(sizeof(T) * N))
