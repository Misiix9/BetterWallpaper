#pragma once
#include <string>
#include <functional>
#include <vector>
#include <atomic>
#include "../utils/InteractiveProcess.hpp"

namespace bwp::steam {

class SteamCMDWorker {
public:
    SteamCMDWorker();
    ~SteamCMDWorker();

    // Login: callback(success, message), twoFactorNeeded()
    // If twoFactorCode is non-empty, it is piped into steamcmd directly.
    void login(const std::string& user, const std::string& pass, 
               std::function<void(bool success, const std::string& msg)> callback,
               std::function<void()> twoFactorNeededCallback,
               const std::string& twoFactorCode = "");
    
    // Auto-login: try using steamcmd's cached session (no password)
    void tryAutoLogin(const std::string& user,
                      std::function<void(bool success, const std::string& msg)> callback);
               
    // Submit 2FA code with its own result callback
    void submitTwoFactorCode(const std::string& code,
                             std::function<void(bool success, const std::string& msg)> callback);
    
    // Download (independent of login state)
    void download(const std::string& workshopId, 
                  std::function<void(float progress)> progressCallback,
                  std::function<void(bool success)> finishedCallback);

    bool isDownloading() const { return m_downloading.load(); }
    bool isLoggingIn() const { return m_loggingIn.load(); }
    void cancel();

private:
    bwp::utils::InteractiveProcess m_process; // kept for potential future use
    std::atomic<bool> m_loggingIn{false};
    std::atomic<bool> m_downloading{false};
    
    std::string m_currentUser;
    std::string m_currentPass; // stored temporarily for 2FA Phase 2

    static std::string shellEscape(const std::string& s);
};

}
