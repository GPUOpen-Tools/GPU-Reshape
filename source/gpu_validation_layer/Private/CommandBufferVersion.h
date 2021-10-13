#pragma once

#include "Descriptor.h"

struct SDescriptorSetVersion
{
    /**
     * Flush this version
     */
    void Flush()
    {
        m_Descriptors.clear();
    }

    /**
     * Accept a set of descriptor writes
     * @param writes the descriptors to accept
     */
    void Accept(const std::vector<STrackedWrite>& writes)
    {
        m_Descriptors.assign(writes.begin(), writes.end());
    }

    /// Get a specific binding within this set
    /// \param binding the binding index
    /// \return the tracked write information
    const STrackedWrite& GetBinding(uint32_t binding) const
    {
        return m_Descriptors[binding];
    }

private:
    std::vector<STrackedWrite> m_Descriptors; ///< Currently tracked descriptors
};

struct SCommandBufferVersion
{
    /**
     * Flush this version
     */
    void Flush()
    {
        for (SDescriptorSetVersion& set : m_DescriptorSets)
        {
            set.Flush();
        }
    }

    /**
     * Get a specific descriptor set within this command list
     * @param index the set index
     * @return the descriptor set version
     */
    SDescriptorSetVersion& GetDescriptorSet(uint32_t index)
    {
        return m_DescriptorSets[index];
    }

private:
    SDescriptorSetVersion m_DescriptorSets[kMaxBoundDescriptorSets]; ///< Currently tracked sets
};