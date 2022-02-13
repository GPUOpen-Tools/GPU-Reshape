#include <Bridge/Asio/AsioDebug.h>
#include <Bridge/Asio/AsioProtocol.h>

// Std
#include <iostream>

void AsioDebugMessage(const AsioHeader* header) {
    std::cout << ToString(header->type) << std::endl;
}
