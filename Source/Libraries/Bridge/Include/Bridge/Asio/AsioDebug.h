#pragma once

/// Asio header debugging
#ifndef ASIO_DEBUG
#    define ASIO_DEBUG 0
#endif

/// Asio content debugging
#ifndef ASIO_CONTENT_DEBUG
#    define ASIO_CONTENT_DEBUG 0
#endif

#if ASIO_DEBUG
// Forward declarations
struct AsioHeader;

/// Debug callback for header
void AsioDebugMessage(const AsioHeader* header);
#endif
