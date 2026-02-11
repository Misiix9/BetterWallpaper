#pragma once
#include "IIPCClient.hpp"
#ifdef _WIN32
#include "NamedPipeClient.hpp"
#else
#include "LinuxIPCClient.hpp"
#endif
#include <memory>
namespace bwp::ipc {
class IPCClientFactory {
public:
    static std::unique_ptr<IIPCClient> createClient() {
#ifdef _WIN32
        return std::make_unique<NamedPipeClient>();
#else
        return std::make_unique<LinuxIPCClient>();
#endif
    }
};
}  
