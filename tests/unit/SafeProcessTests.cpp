#include <gtest/gtest.h>
#include "core/utils/SafeProcess.hpp"
#include <filesystem>
#include <fstream>

using bwp::utils::SafeProcess;

// ──────────────────────────────────────────────────────────
//  SafeProcess::exec()
// ──────────────────────────────────────────────────────────

TEST(SafeProcessExec, EmptyArgsReturnsError) {
  auto result = SafeProcess::exec({});
  EXPECT_NE(result.exitCode, 0);
}

TEST(SafeProcessExec, EchoReturnsOutput) {
  auto result = SafeProcess::exec({"echo", "hello"});
  EXPECT_EQ(result.exitCode, 0);
  EXPECT_TRUE(result.success());
  EXPECT_EQ(result.stdOut, "hello");
}

TEST(SafeProcessExec, NonExistentCommandFails) {
  auto result = SafeProcess::exec({"__nonexistent_binary_xyz__"});
  EXPECT_NE(result.exitCode, 0);
  EXPECT_FALSE(result.success());
}

TEST(SafeProcessExec, FalseCommandReturnsNonZero) {
  auto result = SafeProcess::exec({"false"});
  EXPECT_NE(result.exitCode, 0);
  EXPECT_FALSE(result.success());
}

TEST(SafeProcessExec, TrueCommandReturnsZero) {
  auto result = SafeProcess::exec({"true"});
  EXPECT_EQ(result.exitCode, 0);
  EXPECT_TRUE(result.success());
}

TEST(SafeProcessExec, PathsWithSpacesAreNotShellInterpreted) {
  // Verify that spaces in arguments don't cause word splitting
  auto result = SafeProcess::exec({"echo", "hello world"});
  EXPECT_EQ(result.exitCode, 0);
  EXPECT_EQ(result.stdOut, "hello world");
}

TEST(SafeProcessExec, SpecialCharactersArePassedLiterally) {
  // Verify shell metacharacters don't get interpreted
  auto result = SafeProcess::exec({"echo", "test$(whoami)test"});
  EXPECT_EQ(result.exitCode, 0);
  EXPECT_EQ(result.stdOut, "test$(whoami)test");
}

TEST(SafeProcessExec, MultipleArguments) {
  auto result = SafeProcess::exec({"echo", "one", "two", "three"});
  EXPECT_EQ(result.exitCode, 0);
  EXPECT_EQ(result.stdOut, "one two three");
}

// ──────────────────────────────────────────────────────────
//  SafeProcess::exec() with env vars
// ──────────────────────────────────────────────────────────

TEST(SafeProcessExecEnv, EnvVarsAreSet) {
  auto result = SafeProcess::exec(
      {"sh", "-c", "echo $BWP_TEST_VAR"},
      {"BWP_TEST_VAR=hello_from_test"});
  EXPECT_EQ(result.exitCode, 0);
  EXPECT_EQ(result.stdOut, "hello_from_test");
}

TEST(SafeProcessExecEnv, EmptyEnvVarsDelegatesToSimple) {
  auto result = SafeProcess::exec({"echo", "test"}, {});
  EXPECT_EQ(result.exitCode, 0);
  EXPECT_EQ(result.stdOut, "test");
}

// ──────────────────────────────────────────────────────────
//  SafeProcess::execDetached()
// ──────────────────────────────────────────────────────────

TEST(SafeProcessExecDetached, EmptyArgsFails) {
  EXPECT_FALSE(SafeProcess::execDetached({}));
}

TEST(SafeProcessExecDetached, LaunchesProcess) {
  // Create a temp file, launch a detached process that writes to it,
  // then check if the file was written
  auto tmpFile = std::filesystem::temp_directory_path() / "bwp_test_detached.txt";
  std::filesystem::remove(tmpFile);

  bool launched = SafeProcess::execDetached(
      {"sh", "-c", "echo detached > " + tmpFile.string()});
  EXPECT_TRUE(launched);

  // Give the detached process time to complete
  usleep(500000); // 500ms

  EXPECT_TRUE(std::filesystem::exists(tmpFile));
  if (std::filesystem::exists(tmpFile)) {
    std::ifstream f(tmpFile);
    std::string content;
    std::getline(f, content);
    EXPECT_EQ(content, "detached");
  }

  std::filesystem::remove(tmpFile);
}

// ──────────────────────────────────────────────────────────
//  SafeProcess::commandExists()
// ──────────────────────────────────────────────────────────

TEST(SafeProcessCommandExists, FindsCommonCommands) {
  EXPECT_TRUE(SafeProcess::commandExists("sh"));
  EXPECT_TRUE(SafeProcess::commandExists("echo"));
  EXPECT_TRUE(SafeProcess::commandExists("ls"));
}

TEST(SafeProcessCommandExists, DoesNotFindFakeCommands) {
  EXPECT_FALSE(SafeProcess::commandExists("__nonexistent_binary_xyz__"));
}

// ──────────────────────────────────────────────────────────
//  SafeProcess::shellEscape()
// ──────────────────────────────────────────────────────────

TEST(SafeProcessShellEscape, SimpleString) {
  EXPECT_EQ(SafeProcess::shellEscape("hello"), "'hello'");
}

TEST(SafeProcessShellEscape, StringWithSpaces) {
  EXPECT_EQ(SafeProcess::shellEscape("hello world"), "'hello world'");
}

TEST(SafeProcessShellEscape, StringWithSingleQuotes) {
  EXPECT_EQ(SafeProcess::shellEscape("it's"), "'it'\\''s'");
}

TEST(SafeProcessShellEscape, EmptyString) {
  EXPECT_EQ(SafeProcess::shellEscape(""), "''");
}

TEST(SafeProcessShellEscape, StringWithShellMetacharacters) {
  EXPECT_EQ(SafeProcess::shellEscape("$(rm -rf /)"), "'$(rm -rf /)'");
}
