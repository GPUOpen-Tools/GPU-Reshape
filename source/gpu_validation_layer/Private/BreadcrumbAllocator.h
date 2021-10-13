#pragma once

#include "Common.h"
#include <vector>
#include <atomic>

using BreadcrumbID = TExplicitID<uint64_t, struct TBreadcrumbIdentifier>;

template<typename T>
struct BreadcrumbAllocation
{
    T* operator->() const
    {
        return m_Data;
    }

    BreadcrumbID m_Identifier; ///< ID associated with this allocation
    T*           m_Data;       ///< Allocation user data
};

template<typename T, size_t BLOCK_SIZE = 1024>
class BreadcrumbAllocator
{
public:
    /**
     * Pop a new allocation
     * ! May contain contents of a previous allocation
     */
    BreadcrumbAllocation<T> PopAllocation()
    {
        Lock();

        if (!m_StaleAllocations.empty())
        {
            BreadcrumbAllocation<T> alloc = m_StaleAllocations.back();
            m_StaleAllocations.pop_back();
            Unlock();

            return alloc;
        }

        // Prepare allocation
        BreadcrumbAllocation<T> alloc;
        alloc.m_Identifier = BreadcrumbID(m_Size++);
        alloc.m_Data = AcquireUnsafeElement(alloc.m_Identifier);
        Unlock();

        // Initialize acquired element
        new (alloc.m_Data) T();

        return alloc;
    }

    /**
     * Pop a new allocation
     */
    BreadcrumbAllocation<T> PopFlushedAllocation()
    {
        BreadcrumbAllocation<T> allocation = PopAllocation();
        *allocation.m_Data = T();
        return allocation;
    }

    /**
     * Get an allocation from an ID
     * @param[in] id the allocation id
     */
    BreadcrumbAllocation<T> GetAllocation(BreadcrumbID id)
    {
        BreadcrumbAllocation<T> alloc;
        alloc.m_Identifier = id;

        Lock();
        alloc.m_Data = AcquireUnsafeElement(id);
        Unlock();

        return alloc;
    }

    /**
     * Free an allocation
     * @param[in] alloc the allocation to be free'd
     */
    void FreeAllocation(const BreadcrumbAllocation<T>& alloc)
    {
        Lock();
        m_StaleAllocations.push_back(alloc);
        Unlock();
    }

private:
    /**
     * Find the block index belonging to a linear index
     * @param[in] index the index of the element
     */
    uint64_t ToBlockIndex(uint64_t index)
    {
        return (index - (index % BLOCK_SIZE)) / BLOCK_SIZE;
    }

    /**
     * Get the element pointer from a linear index
     * ! Allocates missing blocks if not present
     * @param[in] id the breadcrumb (linear) id
     */
    T* AcquireUnsafeElement(BreadcrumbID id)
    {
        uint64_t block_index = ToBlockIndex(id.m_ID);

        if (block_index >= m_Blocks.size())
        {
			size_t size = m_Blocks.size();

            m_Blocks.resize(block_index + 1);
            for (uint64_t i = size; i < m_Blocks.size(); i++)
            {
                m_Blocks[i] = new SBlock();
            }
        }

        return &m_Blocks[block_index]->m_Allocations[id.m_ID - block_index * BLOCK_SIZE];
    }

    /**
     * Get the element pointer from a linear index
     * ! Doesn't perform bounds checking
     * @param[in] id the breadcrumb (linear) id
     */
    T* AcquireBoundedElement(BreadcrumbID id)
    {
        uint64_t block_index = ToBlockIndex(id.m_ID);
        return &m_Blocks[block_index]->m_Allocations[id.m_ID - block_index * BLOCK_SIZE];
    }

    /**
     * Lock this allocator
     */
    void Lock()
    {
		while (m_Lock.exchange(true, std::memory_order_acquire));
    }

    /**
     * Unlock this allocator
     */
    void Unlock()
    {
        m_Lock.store(false, std::memory_order_release);
    }

    struct SBlock
    {
        T m_Allocations[BLOCK_SIZE];
    };

private:
    std::atomic<bool>                    m_Lock{false}; ///< Per allocator spin lock
    size_t                               m_Size = 0;         ///< Current number of allocations (including stale allocations)
    std::vector<SBlock *>                m_Blocks;           ///< All blocks
    std::vector<BreadcrumbAllocation<T>> m_StaleAllocations; ///< All stale allocations ready for use
};