#include "SafeProcess.hpp"
#include "Logger.hpp"
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace bwp::utils {

// ──────────────────────────────────────────────────────────
//  exec() — synchronous, stdout captured
// ──────────────────────────────────────────────────────────
SafeProcess::Output
SafeProcess::exec(const std::vector<std::string> &args) {
  if (args.empty()) {
    return {-1, "", "Empty argument list"};
  }

#ifdef _WIN32
  // Windows: build command line for CreateProcess
  // CreateProcess doesn't use shell, but we need a flat command string.
  // Quote each argument that contains spaces.
  std::string cmdLine;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      cmdLine += ' ';
    if (args[i].find(' ') != std::string::npos ||
        args[i].find('\t') != std::string::npos) {
      cmdLine += '"' + args[i] + '"';
    } else {
      cmdLine += args[i];
    }
  }

  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;

  HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
  if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
    return {-1, "", "CreatePipe failed"};
  }
  SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = hWritePipe;
  si.hStdError = hWritePipe;

  PROCESS_INFORMATION pi{};
  std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
  cmdBuf.push_back('\0');

  BOOL ok = CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE, 0,
                           nullptr, nullptr, &si, &pi);
  CloseHandle(hWritePipe);

  if (!ok) {
    CloseHandle(hReadPipe);
    return {-1, "", "CreateProcess failed"};
  }

  // Read stdout
  std::string output;
  char buf[4096];
  DWORD bytesRead;
  while (ReadFile(hReadPipe, buf, sizeof(buf) - 1, &bytesRead, nullptr) &&
         bytesRead > 0) {
    buf[bytesRead] = '\0';
    output += buf;
  }
  CloseHandle(hReadPipe);

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode = 1;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  if (!output.empty() && output.back() == '\n')
    output.pop_back();

  return {static_cast<int>(exitCode), output, ""};

#else
  // POSIX: fork + execvp with a pipe for stdout
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    return {-1, "", std::string("pipe(): ") + strerror(errno)};
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(pipefd[0]);
    close(pipefd[1]);
    return {-1, "", std::string("fork(): ") + strerror(errno)};
  }

  if (pid == 0) {
    // ── Child ──
    close(pipefd[0]); // close read end

    // Redirect stdout + stderr to pipe write end
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);

    // Build argv
    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (const auto &a : args) {
      argv.push_back(const_cast<char *>(a.c_str()));
    }
    argv.push_back(nullptr);

    execvp(argv[0], argv.data());
    // If we get here, exec failed
    _exit(127);
  }

  // ── Parent ──
  close(pipefd[1]); // close write end

  std::string output;
  char buf[4096];
  ssize_t n;
  while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
    buf[n] = '\0';
    output += buf;
  }
  close(pipefd[0]);

  int status = 0;
  waitpid(pid, &status, 0);

  int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

  if (!output.empty() && output.back() == '\n')
    output.pop_back();

  return {exitCode, output, ""};
#endif
}

// ──────────────────────────────────────────────────────────
//  exec() with env vars — synchronous, stdout captured
// ──────────────────────────────────────────────────────────
SafeProcess::Output
SafeProcess::exec(const std::vector<std::string> &args,
                  const std::vector<std::string> &envVars) {
  if (args.empty()) {
    return {-1, "", "Empty argument list"};
  }
  if (envVars.empty()) {
    return exec(args); // delegate to the simpler overload
  }

#ifdef _WIN32
  // On Windows, build an environment block and use the same CreateProcess path
  // For now, delegate to the no-env version (env vars are less common on Win)
  return exec(args);
#else
  // POSIX: fork + execvpe (or manual setenv + execvp) with a pipe for stdout
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    return {-1, "", std::string("pipe(): ") + strerror(errno)};
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(pipefd[0]);
    close(pipefd[1]);
    return {-1, "", std::string("fork(): ") + strerror(errno)};
  }

  if (pid == 0) {
    // ── Child ──
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);

    // Set extra environment variables in the child before exec
    for (const auto &env : envVars) {
      auto eq = env.find('=');
      if (eq != std::string::npos) {
        setenv(env.substr(0, eq).c_str(), env.substr(eq + 1).c_str(), 1);
      }
    }

    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (const auto &a : args) {
      argv.push_back(const_cast<char *>(a.c_str()));
    }
    argv.push_back(nullptr);

    execvp(argv[0], argv.data());
    _exit(127);
  }

  // ── Parent ──
  close(pipefd[1]);

  std::string output;
  char buf[4096];
  ssize_t n;
  while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
    buf[n] = '\0';
    output += buf;
  }
  close(pipefd[0]);

  int status = 0;
  waitpid(pid, &status, 0);
  int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

  if (!output.empty() && output.back() == '\n')
    output.pop_back();

  return {exitCode, output, ""};
#endif
}

// ──────────────────────────────────────────────────────────
//  execDetached() — async, double-fork, setsid
// ──────────────────────────────────────────────────────────
bool SafeProcess::execDetached(const std::vector<std::string> &args,
                               const std::vector<std::string> &envVars) {
  if (args.empty())
    return false;

#ifdef _WIN32
  std::string cmdLine;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      cmdLine += ' ';
    if (args[i].find(' ') != std::string::npos) {
      cmdLine += '"' + args[i] + '"';
    } else {
      cmdLine += args[i];
    }
  }

  STARTUPINFOA si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
  cmdBuf.push_back('\0');

  BOOL ok = CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE,
                           DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, nullptr,
                           nullptr, &si, &pi);
  if (ok) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
  }
  return false;

#else
  pid_t pid = fork();
  if (pid < 0)
    return false;

  if (pid == 0) {
    // First child — setsid to detach from terminal
    setsid();

    // Double fork to orphan the grandchild
    pid_t pid2 = fork();
    if (pid2 < 0)
      _exit(1);
    if (pid2 > 0)
      _exit(0); // first child exits immediately

    // ── Grandchild (fully detached) ──
    // Set extra env vars
    for (const auto &env : envVars) {
      putenv(const_cast<char *>(env.c_str()));
    }

    // Close standard fds
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Build argv
    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (const auto &a : args) {
      argv.push_back(const_cast<char *>(a.c_str()));
    }
    argv.push_back(nullptr);

    execvp(argv[0], argv.data());
    _exit(127);
  }

  // Parent — wait for first child to exit
  int status;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
#endif
}

// ──────────────────────────────────────────────────────────
//  commandExists()
// ──────────────────────────────────────────────────────────
bool SafeProcess::commandExists(const std::string &name) {
#ifdef _WIN32
  // Use SearchPathA to avoid shell
  char buf[MAX_PATH];
  DWORD r = SearchPathA(nullptr, name.c_str(), ".exe", MAX_PATH, buf, nullptr);
  return r > 0;
#else
  // Use execvp("which", ...) through exec()
  auto result = exec({"which", name});
  return result.exitCode == 0;
#endif
}

// ──────────────────────────────────────────────────────────
//  shellEscape() — POSIX single-quote escaping
// ──────────────────────────────────────────────────────────
std::string SafeProcess::shellEscape(const std::string &input) {
  // Wrap in single quotes. The only character that needs escaping
  // inside single quotes is the single quote itself:
  //   ' → '\''  (end quote, escaped quote, start quote)
  std::string result = "'";
  for (char c : input) {
    if (c == '\'') {
      result += "'\\''";
    } else {
      result += c;
    }
  }
  result += "'";
  return result;
}

} // namespace bwp::utils
