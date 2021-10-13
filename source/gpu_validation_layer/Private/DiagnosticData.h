#pragma once

#include "Common.h"
#include <vector>
#include <mutex>

static constexpr auto kMaxMessageTypes = 64;
static constexpr auto kMessageHeaderBits = 6;
static constexpr auto kMessageBodyBits = 32 - kMessageHeaderBits;

// Represents a diagnostics message
// Structure mirrored by the GPU
struct SDiagnosticMessageData
{
	/**
	 * Construct a message with given contents
	 * @param[in] type the message type
	 * @param[in] header the mesage header
	 */
	template<typename T>
	static SDiagnosticMessageData Construct(uint64_t type, T&& message)
	{
		static_assert(sizeof(T) <= 26, "Message must be <= 26");

		uint32_t msg32;
		*reinterpret_cast<std::remove_reference_t<T>*>(&msg32) = message;

		SDiagnosticMessageData data;
		data.m_Type = type;
		data.m_Message = ToMessage(message);
		return data;
	}

	/**
	 * Composite a header key
	 * @param[in] type the message type
	 * @param[in] header the mesage header
	 */
	template<typename T>
	static uint32_t ToMessage(T&& header)
	{
		uint32_t message32;
		*reinterpret_cast<std::remove_reference_t<T>*>(&message32) = header;

		return message32;
	}

	/**
	 * Cast the message
	 */
	template<typename T>
	T GetMessage() const
	{
		static_assert(sizeof(T) <= 26, "Message must be <= 26");
		uint32_t copy = m_Message;
		return *reinterpret_cast<const T*>(&copy);
	}

	/**
	 * Get the whole key of this message
	 */
	uint32_t GetKey() const
	{
		return *reinterpret_cast<const uint32_t*>(this);
	}

	uint32_t m_Type : 6;     ///< 64 different message types
	uint32_t m_Message : 26; ///< The message contents
};

static_assert(sizeof(SDiagnosticMessageData) == sizeof(uint32_t), "Unexpected size");

// Represents an allocation
// Structure shared by the GPU
struct SDiagnosticData
{
	uint32_t				m_MessageCount;			   ///< Number of validation messages, atomically incremented
	uint32_t				m_MessageLimit;			   ///< Constant limit of validation messages
	uint32_t				m_TransferredMessageCount; ///< The actual number of transferred messages
	uint32_t				m_Debug;				   ///< Debug value, doing debuggy thigngs for debugging purposes

	SDiagnosticMessageData  m_Messages[1]; ///< Message contents, variable array counts
};

static_assert(sizeof(SDiagnosticData) == sizeof(uint32_t) * 4 + sizeof(SDiagnosticMessageData), "Unexpected size");
