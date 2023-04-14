#pragma once

using namespace System;

namespace Discovery::CLR {
    public ref class DiscoveryProcessInfo {
    public:
        /// Path of the application
        String^ applicationPath = "";

        /// Working directory of the application
        String^ workingDirectoryPath = "";

        /// All command line arguments given to the application
        String^ arguments = "";
    };
}
