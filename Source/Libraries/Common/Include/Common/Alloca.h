#pragma once

// Std
#include <malloc.h>

// Stack allocation
#ifdef _MSC_VER
#	define ALLOCA(T) static_cast<T*>(_alloca(sizeof(T)))
#	define ALLOCA_SIZE(T, N) static_cast<T*>(_alloca(N))
#	define ALLOCA_ARRAY(T, N) static_cast<T*>(_alloca(sizeof(T) * (N)))
#else
#	define ALLOCA(T) static_cast<T*>(alloca(sizeof(T)))
#	define ALLOCA_SIZE(T, N) static_cast<T*>(alloca(N))
#	define ALLOCA_ARRAY(T, N) static_cast<T*>(alloca(sizeof(T) * (N)))
#endif
