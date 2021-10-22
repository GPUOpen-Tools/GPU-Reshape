#pragma once

#define NOMINMAX

#include <vulkan_layers/gpu_validation_layer.h>
#include <vulkan/vk_layer.h>
#include <functional>
#include <atomic>

// Cleanup global scope preprocessors
#undef Bool
#undef Status
#undef Process
#undef GetMessage
#undef OPAQUE

//#pragma optimize ("", off)

// Linkage information
#if defined(_WIN32)
#	define AVA_EXPORT __declspec(dllexport)
#	define AVA_C_EXPORT extern "C" __declspec(dllexport)
#else
#	define AVA_EXPORT __attribute__((visibility("default")))
#	define AVA_C_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// Alloca helper
#define ALLOCA_T(TYPE) (static_cast<TYPE*>(alloca(sizeof(TYPE))))

// Alloca array helper
#define ALLOCA_ARRAY(TYPE, COUNT) (static_cast<TYPE*>(alloca(sizeof(TYPE) * (COUNT))))

/**
 * Find a chained structure type
 * @param[in] chain the source chain
 * @param[in] type the structure type to be searched for
 */
template<typename T, typename U>
inline const T* FindStructureType(const U* chain, uint64_t type) {
	const U* current = chain;
	while (current && current->sType != type)
	{
		current = reinterpret_cast<const U*>(current->pNext);
	}
	return reinterpret_cast<const T*>(current);
}

/**
 * Combine a hash value
 * @param[out] the source and destination hash
 * @param[in] value the hash to be overlayed
 */
template <class T>
inline void CombineHash(std::size_t& hash, T value)
{
	std::hash<T> hasher;
	hash ^= (hasher(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
}

/**
 * Align a value
 * @param[in] value the value to be aligned
 * @param[in] alignment the alignment to be applied
 */
inline uint64_t AlignUpper(uint64_t value, uint64_t alignment)
{
	size_t align_offset = value % alignment;
	return align_offset ? (value + (alignment - align_offset)) : value;
}

/**
 * Format a buffer according to specification
 * @param[in] buffer the fixed length buffer to format into
 * @param[in] format the format specification
 * @param[in] args the arguments for the specification
 */
template<size_t LENGTH, typename... A>
inline void FormatBuffer(char (&buffer)[LENGTH], const char* format, A&&... args)
{
	snprintf(buffer, LENGTH, format, args...);
}

// A sparse counter, useful for intervaled messages
struct SSparseCounter
{
	/**
	 * Increment this counter
	 * @param[in] threshold the counter value at which it returns true and is reset
	 */
	bool Next(uint32_t threshold)
	{
		return (m_Counter = ((m_Counter + 1) % threshold)) == 0;
	}

private:
	uint32_t m_Counter = 0;
};

// A strongly typed id
template<typename T, typename OPAQUE>
struct TExplicitID
{
    explicit TExplicitID(T id = T {}) : m_ID(id)
    {
        /* poof */
    }

    T m_ID;
};

// Default ownership destruction
template <typename T>
struct TDefaultOwnershipDestructor
{
	void operator()(T* resource) const
	{
		delete resource;
	}
};

// Helper for deferred ownership destruction
template<typename T, typename DESTRUCTOR = TDefaultOwnershipDestructor<T>>
struct TDeferredOwnership
{
	void Release()
	{
		if (--m_Usages == 0)
		{
			DESTRUCTOR{}(static_cast<T*>(this));
		}
	}

	void Acquire()
	{
		m_Usages++;
	}

	std::atomic<uint32_t> m_Usages{ 1 };
};

// Get the size of a Vulkan format
inline uint32_t FormatToSize(VkFormat format)
{
    switch (format)
    {
        default:
            return 0;
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
            return 4;
        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
            return 8;
    }
}

// Internal limits
static constexpr uint32_t kMaxBoundDescriptorSets = 32;
static constexpr uint32_t kTrackedPipelineBindPoints = 2;