#pragma once
#include "IIPCService.hpp"
#ifdef _WIN32
#include "NamedPipeService.hpp"
#else
#include "DBusService.hpp"
#endif
#include <memory>
namespace bwp::ipc {
class IPCServiceFactory {
public:
    static std::unique_ptr<IIPCService> createService() {
#ifdef _WIN32
        return std::make_unique<NamedPipeService>();
#else
        return std::make_unique<DBusService>();
#endif
    }
};
}  
