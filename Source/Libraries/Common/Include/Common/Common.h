#pragma once

// Export
#ifdef _WIN32
#  define DLL_EXPORT __declspec(dllexport)
#else // _WIN32
#  define DLL_EXPORT
#endif // _WIN64

// C-Export
#define DLL_EXPORT_C extern "C" DLL_EXPORT

#if __cplusplus >= 202002L
#define GRS_CONSTEVAL consteval
#else // __cplusplus >= 202002L
#define GRS_CONSTEVAL constexpr
#endif // __cplusplus >= 202002L
