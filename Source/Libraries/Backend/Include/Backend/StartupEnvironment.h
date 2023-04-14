#pragma once

// Message
#include <Message/MessageStream.h>

// Std
#include <string>

namespace Backend {
    struct StartupEnvironment {
        /// Expected environment key
        static constexpr const char* kEnvKey = "GPUOpen.GRS.StartupEnvironment";

        /// Populate a stream from the current environment
        /// \param stream destination stream
        /// \return success code
        bool LoadFromEnvironment(MessageStream& stream);

        /// Write a startup environment
        /// \param stream given stream to write
        /// \return final path
        std::string WriteEnvironment(const MessageStream& stream);
    };
}
