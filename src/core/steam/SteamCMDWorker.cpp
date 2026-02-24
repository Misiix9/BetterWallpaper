#include "SteamCMDWorker.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/Logger.hpp"
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <regex>
#include <sstream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace bwp::steam {

// Securely zero a string's contents before clearing, preventing
// password data from lingering in freed heap memory.
static void secureWipe(std::string &s) {
  if (!s.empty()) {
    // Use volatile to prevent the compiler from optimizing away the memset
    volatile char *p = const_cast<volatile char *>(s.data());
    std::memset(const_cast<char *>(s.data()), 0, s.size());
    (void)p; // prevent unused-variable warning
  }
  s.clear();
}

SteamCMDWorker::SteamCMDWorker() {}

SteamCMDWorker::~SteamCMDWorker() {
  cancel();
  secureWipe(m_currentPass);
}

void SteamCMDWorker::cancel() {
  if (m_process.isRunning()) {
    m_process.kill();
  }
  m_loggingIn = false;
  m_downloading = false;
}

std::string SteamCMDWorker::shellEscape(const std::string &s) {
  std::string result;
  for (char c : s) {
    if (c == '\'') {
      result += "'\\''";
    } else {
      result += c;
    }
  }
  return result;
}

// ---------------------------------------------------------------------------
// LOGIN (Phase 1): Try credentials, detect 2FA
// ---------------------------------------------------------------------------
void SteamCMDWorker::login(
    const std::string &user, const std::string &pass,
    std::function<void(bool, const std::string &)> callback,
    std::function<void()> twoFactorNeededCallback,
    const std::string &twoFactorCode) {
  if (m_loggingIn.load()) {
    LOG_WARN("[SteamCMD] login() called but already logging in");
    if (callback)
      callback(false, "Login already in progress");
    return;
  }

  m_currentUser = user;
  m_currentPass = pass;
  m_loggingIn = true;

  LOG_INFO("[SteamCMD] Starting login for user: " + user);

  std::string cmd;
  if (!twoFactorCode.empty()) {
    // Pipe the 2FA code directly into steamcmd
    cmd = "printf '" + shellEscape(twoFactorCode) + "\\n' | steamcmd +login '" +
          shellEscape(user) + "' '" + shellEscape(pass) + "' +quit 2>&1";
    LOG_INFO("[SteamCMD] Login with 2FA code provided upfront");
  } else {
    cmd = "steamcmd +login '" + shellEscape(user) + "' '" + shellEscape(pass) +
          "' +quit < /dev/null 2>&1";
  }

  std::thread([this, cmd, callback, twoFactorNeededCallback]() {
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
      LOG_ERROR("[SteamCMD] Failed to start login process");
      m_loggingIn = false;
      secureWipe(m_currentPass);
      if (callback)
        callback(false, "Failed to start steamcmd");
      return;
    }

    char buf[1024];
    bool loggedIn = false;
    bool needs2FA = false;
    std::string failMsg;
    std::string lastLine; // Track last meaningful output for fallback

    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
      std::string line(buf);
      while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        line.pop_back();
      // Strip ANSI escape codes (steamcmd emits color resets etc.)
      line =
          std::regex_replace(line, std::regex("\\x1B\\[[0-9;]*[A-Za-z]"), "");
      // Trim again after ANSI strip
      while (!line.empty() && (line.back() == ' ' || line.front() == ' ')) {
        if (line.back() == ' ')
          line.pop_back();
        if (!line.empty() && line.front() == ' ')
          line.erase(line.begin());
      }
      if (line.empty())
        continue;

      LOG_DEBUG("[SteamCMD login] " + line);

      // Skip steamcmd boilerplate for lastLine tracking
      if (line.find("Redirecting stderr") == std::string::npos &&
          line.find("Logging directory") == std::string::npos &&
          line.find("Checking for available") == std::string::npos &&
          line.find("Verifying installation") == std::string::npos &&
          line.find("Loading Steam API") == std::string::npos &&
          line.find("Unloading Steam API") == std::string::npos &&
          line.find("Waiting for") == std::string::npos &&
          line.find("UpdateUI") == std::string::npos &&
          line.find("Steam Console Client") == std::string::npos &&
          line.find("type 'quit'") == std::string::npos &&
          line.find("[  0%]") == std::string::npos &&
          line.find("[----]") == std::string::npos)
        lastLine = line;

      if (line.find("Logged in OK") != std::string::npos ||
          line.find("Waiting for user info...OK") != std::string::npos)
        loggedIn = true;
      if (line.find("Two-factor code:") != std::string::npos ||
          line.find("Two-Factor") != std::string::npos ||
          line.find("Steam Guard code:") != std::string::npos)
        needs2FA = true;
      if (line.find("FAILED") != std::string::npos && failMsg.empty())
        failMsg = line;
      if (line.find("Invalid Password") != std::string::npos ||
          line.find("invalid password") != std::string::npos)
        failMsg = "Invalid username or password";
      if (line.find("Login Denied") != std::string::npos ||
          line.find("Account Logon Denied") != std::string::npos)
        failMsg = "Login denied — check your credentials";
      if (line.find("Expired Login Auth Code") != std::string::npos)
        failMsg = "Login auth code has expired";
      if (line.find("InvalidLoginAuthCode") != std::string::npos ||
          line.find("Invalid Login Auth Code") != std::string::npos)
        failMsg = "Invalid login auth code";
      if (line.find("No Connection") != std::string::npos ||
          line.find("no connection") != std::string::npos)
        failMsg = "No connection to Steam — check your network";
      if (line.find("Rate Limit") != std::string::npos ||
          line.find("rate limit") != std::string::npos) {
        pclose(pipe);
        m_loggingIn = false;
        secureWipe(m_currentPass);
        if (callback)
          callback(false, "Rate Limit Exceeded — try again later");
        return;
      }
    }

    int status = pclose(pipe);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    LOG_INFO("[SteamCMD] Login process exited with code " +
             std::to_string(exitCode));

    // Treat clean exit (code 0) with no explicit failure as success
    // (handles mobile authenticator flow and other steamcmd variations)
    if (!loggedIn && !needs2FA && exitCode == 0 && failMsg.empty())
      loggedIn = true;

    // If no specific error matched, use last meaningful line as context
    if (!loggedIn && failMsg.empty() && !lastLine.empty())
      failMsg = lastLine;

    if (loggedIn) {
      LOG_INFO("[SteamCMD] Logged in successfully");
      // Keep m_currentPass for session — needed for download commands
      // to bypass "Waiting for client config" when Steam client is running.
      // Password is wiped on logout/destructor.
      m_loggingIn = false;
      if (callback)
        callback(true, "Logged in OK");
    } else if (needs2FA) {
      LOG_INFO("[SteamCMD] 2FA required, prompting user");
      // Stay m_loggingIn=true for Phase 2; keep m_currentPass for 2FA
      if (twoFactorNeededCallback)
        twoFactorNeededCallback();
    } else {
      LOG_ERROR("[SteamCMD] Login failed: " + failMsg);
      secureWipe(m_currentPass);
      m_loggingIn = false;
      if (callback)
        callback(false, failMsg.empty()
                            ? "Login failed — check your credentials"
                            : failMsg);
    }
  }).detach();
}

// ---------------------------------------------------------------------------
// LOGIN (Phase 2): Submit 2FA code
// ---------------------------------------------------------------------------
void SteamCMDWorker::submitTwoFactorCode(
    const std::string &code,
    std::function<void(bool, const std::string &)> callback) {
  if (!m_loggingIn.load()) {
    LOG_WARN("[SteamCMD] submitTwoFactorCode() called but not in login phase");
    if (callback)
      callback(false, "Not in login phase");
    return;
  }

  LOG_INFO("[SteamCMD] Submitting 2FA code (Phase 2)");

  std::string cmd = "printf '" + shellEscape(code) +
                    "\\n' | steamcmd +login '" + shellEscape(m_currentUser) +
                    "' '" + shellEscape(m_currentPass) + "' +quit 2>&1";

  std::thread([this, cmd, callback]() {
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
      LOG_ERROR("[SteamCMD] Failed to start 2FA login process");
      secureWipe(m_currentPass);
      m_loggingIn = false;
      if (callback)
        callback(false, "Failed to start steamcmd for 2FA");
      return;
    }

    char buf[1024];
    bool loggedIn = false;
    bool wrongCode = false;
    std::string failMsg;

    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
      std::string line(buf);
      while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        line.pop_back();
      if (line.empty())
        continue;

      LOG_DEBUG("[SteamCMD 2FA] " + line);

      if (line.find("Logged in OK") != std::string::npos)
        loggedIn = true;
      if (line.find("FAILED") != std::string::npos)
        failMsg = line;
      if (line.find("TwoFactorCodeMismatch") != std::string::npos ||
          line.find("Invalid") != std::string::npos)
        wrongCode = true;
      if (line.find("Rate Limit") != std::string::npos) {
        pclose(pipe);
        secureWipe(m_currentPass);
        m_loggingIn = false;
        if (callback)
          callback(false, "Rate Limit Exceeded — try again later");
        return;
      }
    }

    pclose(pipe);

    if (loggedIn) {
      LOG_INFO("[SteamCMD] Successfully logged in with 2FA");
      secureWipe(m_currentPass);
      m_loggingIn = false;
      if (callback)
        callback(true, "Logged in OK");
    } else {
      // Keep m_currentPass and m_loggingIn so user can retry 2FA
      LOG_ERROR("[SteamCMD] 2FA login failed: " + failMsg);
      std::string msg = wrongCode
                            ? "Invalid 2FA code — please try again"
                            : (failMsg.empty() ? "2FA login failed" : failMsg);
      if (callback)
        callback(false, msg);
    }
  }).detach();
}

// ---------------------------------------------------------------------------
// AUTO-LOGIN: Try cached credentials (no password)
// ---------------------------------------------------------------------------
void SteamCMDWorker::tryAutoLogin(
    const std::string &user,
    std::function<void(bool, const std::string &)> callback) {
  if (m_loggingIn.load()) {
    LOG_WARN("[SteamCMD] tryAutoLogin() called but already logging in");
    if (callback)
      callback(false, "Login already in progress");
    return;
  }

  m_loggingIn = true;
  LOG_INFO("[SteamCMD] Attempting auto-login for user: " + user);

  // No password — steamcmd will use its internally cached session token
  // Use fake HOME to bypass IPC hang when Steam Desktop is running
  std::string fakeHomeCmd =
      "mkdir -p /tmp/bwp_steamcmd/.steam && "
      "ln -sfn \"$HOME/.steam/steam\" /tmp/bwp_steamcmd/.steam/steam && "
      "ln -sfn \"$HOME/.steam/root\" /tmp/bwp_steamcmd/.steam/root && "
      "env HOME=/tmp/bwp_steamcmd ";

  std::string cmd = fakeHomeCmd + "steamcmd +login '" + shellEscape(user) +
                    "' +quit < /dev/null 2>&1";

  std::thread([this, cmd, callback]() {
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
      LOG_ERROR("[SteamCMD] Failed to start auto-login process");
      m_loggingIn = false;
      if (callback)
        callback(false, "Failed to start steamcmd");
      return;
    }

    char buf[1024];
    bool loggedIn = false;
    bool needs2FA = false;
    std::string failMsg;

    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
      std::string line(buf);
      while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        line.pop_back();
      line =
          std::regex_replace(line, std::regex("\\x1B\\[[0-9;]*[A-Za-z]"), "");
      while (!line.empty() && (line.back() == ' ' || line.front() == ' ')) {
        if (line.back() == ' ')
          line.pop_back();
        if (!line.empty() && line.front() == ' ')
          line.erase(line.begin());
      }
      if (line.empty())
        continue;

      LOG_DEBUG("[SteamCMD auto-login] " + line);

      if (line.find("Logged in OK") != std::string::npos ||
          line.find("Waiting for user info...OK") != std::string::npos)
        loggedIn = true;
      if (line.find("Two-factor code:") != std::string::npos ||
          line.find("Two-Factor") != std::string::npos ||
          line.find("Steam Guard code:") != std::string::npos)
        needs2FA = true;
      if (line.find("FAILED") != std::string::npos && failMsg.empty())
        failMsg = line;
      if (line.find("Invalid Password") != std::string::npos ||
          line.find("Cached credentials not found") != std::string::npos ||
          line.find("Login Denied") != std::string::npos)
        failMsg = line;
    }

    int status = pclose(pipe);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    LOG_INFO("[SteamCMD] Auto-login process exited with code " +
             std::to_string(exitCode));

    // Clean exit with no failure = success (handles mobile auth flow)
    if (!loggedIn && !needs2FA && exitCode == 0 && failMsg.empty())
      loggedIn = true;

    m_loggingIn = false;

    if (loggedIn) {
      LOG_INFO("[SteamCMD] Auto-login succeeded (cached credentials valid)");
      if (callback)
        callback(true, "Auto-login OK");
    } else {
      LOG_INFO("[SteamCMD] Auto-login failed — cached credentials expired or "
               "missing");
      if (callback)
        callback(false,
                 failMsg.empty() ? "Cached credentials expired" : failMsg);
    }
  }).detach();
}

// ---------------------------------------------------------------------------
// DOWNLOAD (completely independent of login state)
// ---------------------------------------------------------------------------
void SteamCMDWorker::download(const std::string &workshopId,
                              std::function<void(float)> progressCallback,
                              std::function<void(bool)> finishedCallback) {
  if (m_downloading.load()) {
    LOG_WARN("[SteamCMD] download() called but already downloading");
    if (finishedCallback)
      finishedCallback(false);
    return;
  }

  m_downloading = true;

  // Determine login user: cached from login, config, or anonymous
  std::string loginUser = m_currentUser;
  if (loginUser.empty()) {
    loginUser = bwp::config::ConfigManager::getInstance().get<std::string>(
        "steam_user", "");
  }
  if (loginUser.empty()) {
    loginUser = "anonymous";
  }

  LOG_INFO("[SteamCMD] Starting download of " + workshopId + " as " +
           loginUser);

  // Use fake HOME to bypass IPC hang when Steam Desktop is running.
  // This avoids passing the password repeatedly, which triggers 2FA loops.
  std::string fakeHomeCmd =
      "mkdir -p /tmp/bwp_steamcmd/.steam && "
      "ln -sfn \"$HOME/.steam/steam\" /tmp/bwp_steamcmd/.steam/steam && "
      "ln -sfn \"$HOME/.steam/root\" /tmp/bwp_steamcmd/.steam/root && "
      "env HOME=/tmp/bwp_steamcmd ";

  std::string cmd = fakeHomeCmd + "steamcmd +login '" + shellEscape(loginUser) +
                    "' +workshop_download_item 431960 " + workshopId +
                    " +quit 2>&1";

  std::thread([this, cmd, workshopId, loginUser, progressCallback,
               finishedCallback]() {
    auto runSteamCmd =
        [&](const std::string &command) -> std::pair<bool, bool> {
      // Use pipe + fork + exec so we can poll() with a timeout
      // and detect when steamcmd hangs (e.g. "Waiting for client config...")
      int pipefd[2];
      if (pipe(pipefd) != 0) {
        LOG_ERROR("[SteamCMD] Failed to create pipe");
        return {false, false};
      }

      pid_t pid = fork();
      if (pid < 0) {
        LOG_ERROR("[SteamCMD] Failed to fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return {false, false};
      }

      if (pid == 0) {
        // Child: redirect stdout+stderr to write-end of pipe, close read-end
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        // Redirect stdin from /dev/null
        int devnull = open("/dev/null", 0);
        if (devnull >= 0) {
          dup2(devnull, STDIN_FILENO);
          close(devnull);
        }
        execl("/bin/sh", "sh", "-c", command.c_str(), (char *)nullptr);
        _exit(127);
      }

      // Parent: read from pipe with poll() timeout
      close(pipefd[1]); // close write end

      bool success = false;
      bool needsFallback = false;
      std::regex rePercent(R"(progress:\s*(\d+(?:\.\d+)?))");
      std::regex reBytes(R"(\((\d+)\s*/\s*(\d+)\))");
      std::string lineBuf;

      constexpr int TIMEOUT_MS = 30000; // 30s no-output timeout
      struct pollfd pfd;
      pfd.fd = pipefd[0];
      pfd.events = POLLIN;

      bool timedOut = false;
      bool clientConfigHang = false;
      char buf[1024];

      while (true) {
        int ret = poll(&pfd, 1, TIMEOUT_MS);
        if (ret == 0) {
          // Timeout — no output for 30s, steamcmd is likely hung
          LOG_WARN(
              "[SteamCMD] No output for 30s — killing hung steamcmd process");
          timedOut = true;
          break;
        }
        if (ret < 0) {
          if (errno == EINTR)
            continue;
          LOG_ERROR("[SteamCMD] poll() error: " + std::string(strerror(errno)));
          break;
        }
        ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
        if (n <= 0)
          break; // EOF or error
        buf[n] = '\0';
        lineBuf += buf;

        // Process complete lines
        size_t pos;
        while ((pos = lineBuf.find('\n')) != std::string::npos) {
          std::string line = lineBuf.substr(0, pos);
          lineBuf.erase(0, pos + 1);
          while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
          if (line.empty())
            continue;

          LOG_DEBUG("[SteamCMD] " + line);

          // Detect the known hang condition - ONLY if "OK" is not present
          if (line.find("Waiting for client config") != std::string::npos &&
              line.find("OK") == std::string::npos) {
            LOG_WARN(
                "[SteamCMD] Detected 'Waiting for client config' hang — Steam "
                "client conflict");
            clientConfigHang = true;
            break;
          }

          if (line.find("Cached credentials not found") != std::string::npos) {
            LOG_WARN("[SteamCMD] No cached credentials, will retry anonymous");
            needsFallback = true;
            break;
          }

          if (line.find("progress:") != std::string::npos) {
            std::smatch match;
            bool gotProgress = false;

            if (std::regex_search(line, match, rePercent)) {
              try {
                float prog = std::stof(match[1].str());
                if (progressCallback)
                  progressCallback(prog / 100.0f);
                gotProgress = true;
              } catch (const std::exception &e) {
                LOG_DEBUG("[SteamCMD] Failed to parse progress percent: " +
                          std::string(e.what()));
              }
            }

            if (!gotProgress && std::regex_search(line, match, reBytes)) {
              try {
                double dl = std::stod(match[1].str());
                double total = std::stod(match[2].str());
                if (total > 0) {
                  if (progressCallback)
                    progressCallback(static_cast<float>(dl / total));
                }
              } catch (const std::exception &e) {
                LOG_DEBUG("[SteamCMD] Failed to parse progress bytes: " +
                          std::string(e.what()));
              }
            }
          }

          if (line.find("Success. Downloaded item") != std::string::npos) {
            LOG_INFO("[SteamCMD] Download success: " + line);
            success = true;
            if (progressCallback)
              progressCallback(1.0f);
          }

          if (line.find("ERROR") != std::string::npos ||
              line.find("FAILED") != std::string::npos) {
            LOG_ERROR("[SteamCMD] Download error: " + line);
          }
        }

        if (needsFallback || clientConfigHang)
          break;
      }

      close(pipefd[0]);

      // If hung or timed out, kill the child process
      if (timedOut || clientConfigHang) {
        kill(pid, SIGTERM);
        usleep(200000); // 200ms grace
        int wstatus;
        if (waitpid(pid, &wstatus, WNOHANG) == 0) {
          kill(pid, SIGKILL);
          waitpid(pid, &wstatus, 0);
        }
        if (clientConfigHang) {
          LOG_WARN("[SteamCMD] Detected Steam client conflict. Killing 'steam' "
                   "process...");
          // Kill steam and wait a bit
          system("pkill -9 steam");
          std::this_thread::sleep_for(std::chrono::seconds(2));
          LOG_INFO(
              "[SteamCMD] Retrying download after killing Steam client...");
          return {false, true /* needsFallback / retry */};
        }
        return {false, needsFallback};
      }

      // Wait for child to finish normally
      int wstatus;
      waitpid(pid, &wstatus, 0);
      int exitCode = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;
      LOG_INFO("[SteamCMD] Process exited with code " +
               std::to_string(exitCode));

      return {success, needsFallback};
    };

    auto [success, needsFallback] = runSteamCmd(cmd);

    // If failed due to Steam client conflict or auth, retry
    if (!success && needsFallback) {
      LOG_INFO("[SteamCMD] Retrying download of " + workshopId +
               " (fallback/retry)");
      std::string anonCmd = "steamcmd +login anonymous"
                            " +workshop_download_item 431960 " +
                            workshopId + " +quit < /dev/null 2>&1";
      auto [anonSuccess, _] = runSteamCmd(anonCmd);
      success = anonSuccess;
    }

    m_downloading = false;
    if (finishedCallback)
      finishedCallback(success);
  }).detach();
}

} // namespace bwp::steam
