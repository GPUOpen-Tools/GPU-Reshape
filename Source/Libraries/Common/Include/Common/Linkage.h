#pragma once

#ifdef _WIN32
#  define DLL_EXPORT __declspec(dllexport)
#else
#  define DLL_EXPORT
#endif

#define DLL_EXPORT_C extern "C" DLL_EXPORT
