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
SafeProcess::Output
SafeProcess::exec(const std::vector<std::string> &args) {
  if (args.empty()) {
    return {-1, "", "Empty argument list"};
  }
#ifdef _WIN32
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
    close(pipefd[0]);  
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);
    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (const auto &a : args) {
      argv.push_back(const_cast<char *>(a.c_str()));
    }
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());
    _exit(127);
  }
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
SafeProcess::Output
SafeProcess::exec(const std::vector<std::string> &args,
                  const std::vector<std::string> &envVars) {
  if (args.empty()) {
    return {-1, "", "Empty argument list"};
  }
  if (envVars.empty()) {
    return exec(args);  
  }
#ifdef _WIN32
  return exec(args);
#else
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
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);
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
    setsid();
    pid_t pid2 = fork();
    if (pid2 < 0)
      _exit(1);
    if (pid2 > 0)
      _exit(0);  
    for (const auto &env : envVars) {
      putenv(const_cast<char *>(env.c_str()));
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (const auto &a : args) {
      argv.push_back(const_cast<char *>(a.c_str()));
    }
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());
    _exit(127);
  }
  int status;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
#endif
}
bool SafeProcess::commandExists(const std::string &name) {
#ifdef _WIN32
  char buf[MAX_PATH];
  DWORD r = SearchPathA(nullptr, name.c_str(), ".exe", MAX_PATH, buf, nullptr);
  return r > 0;
#else
  auto result = exec({"which", name});
  return result.exitCode == 0;
#endif
}
std::string SafeProcess::shellEscape(const std::string &input) {
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
}  
