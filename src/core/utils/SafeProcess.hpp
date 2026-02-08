#pragma once

#include <string>
#include <vector>

namespace bwp::utils {

/**
 * @brief Safe process execution without shell interpolation.
 *
 * All methods use fork/execvp (Linux) or CreateProcess (Windows)
 * with an argument array — the shell is never invoked, so user-provided
 * strings (paths, monitor names, etc.) cannot cause injection.
 */
class SafeProcess {
public:
  struct Output {
    int exitCode;
    std::string stdOut;
    std::string stdErr;
    bool success() const { return exitCode == 0; }
  };

  /**
   * @brief Run a program synchronously, capturing stdout.
   * @param args argv array — args[0] is the executable.
   * @return Output with exit code and captured stdout.
   */
  static Output exec(const std::vector<std::string> &args);

  /**
   * @brief Run a program synchronously with extra environment variables.
   * @param args argv array — args[0] is the executable.
   * @param envVars Extra env vars as "KEY=VALUE" pairs (appended to current env).
   * @return Output with exit code and captured stdout.
   */
  static Output exec(const std::vector<std::string> &args,
                     const std::vector<std::string> &envVars);

  /**
   * @brief Run a program asynchronously (fire and forget).
   *
   * The child is double-forked and detached via setsid() so it
   * survives the parent's exit.
   *
   * @param args argv array — args[0] is the executable.
   * @param envVars Optional extra env vars as "KEY=VALUE" pairs.
   * @return true if the child was spawned successfully.
   */
  static bool execDetached(const std::vector<std::string> &args,
                           const std::vector<std::string> &envVars = {});

  /**
   * @brief Check if an executable exists in PATH.
   * @param name Bare executable name (e.g. "mpv").
   */
  static bool commandExists(const std::string &name);

  /**
   * @brief Shell-escape a single string for safe embedding in a shell
   * command. Wraps the string in single quotes and escapes embedded
   * single quotes.  Prefer exec()/execDetached() over shell commands.
   *
   * @param input The raw string to escape.
   * @return Escaped string safe for POSIX shell embedding.
   */
  static std::string shellEscape(const std::string &input);
};

} // namespace bwp::utils
