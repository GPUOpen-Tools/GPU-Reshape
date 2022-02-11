#pragma once

// Stack allocation
#ifdef _MSC_VER
#	define ALLOCA(T) static_cast<T*>(_malloca(sizeof(T)))
#	define ALLOCA_ARRAY(T, N) static_cast<T*>(_malloca(sizeof(T) * (N)))
#else
#	define ALLOCA(T) static_cast<T*>(alloca(sizeof(T)))
#	define ALLOCA_ARRAY(T, N) static_cast<T*>(alloca(sizeof(T) * (N)))
#endif
