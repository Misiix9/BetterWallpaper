#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

// Forward declarations to avoid <gio/gio.h> in header if possible, 
// but GSubprocess* requires it or void*. Let's include gio.
#include <gio/gio.h> 

namespace bwp::utils {

class InteractiveProcess {
public:
    using OutputCallback = std::function<void(const std::string&)>;
    using ExitCallback = std::function<void(int)>;

    InteractiveProcess();
    ~InteractiveProcess();

    // Start the process asynchronously
    bool start(const std::vector<std::string>& args);

    // Write line to stdin
    void writeLine(const std::string& line);

    // Stop cleanly (SIGTERM)
    void stop();

    // Force kill (SIGKILL)
    void kill();

    bool isRunning() const;

    // Callbacks must be thread-safe or aware they are called from GIO loop
    void setStdoutCallback(OutputCallback callback);
    void setStderrCallback(OutputCallback callback);
    void setExitCallback(ExitCallback callback);

private:
    static void onReadStdout(GObject* source, GAsyncResult* res, gpointer user_data);
    static void onReadStderr(GObject* source, GAsyncResult* res, gpointer user_data);
    static void onProcessExited(GObject* source, GAsyncResult* res, gpointer user_data);

    void handleRead(GInputStream* stream, GAsyncResult* res, OutputCallback& callback);

    GSubprocess* m_subprocess = nullptr;
    GOutputStream* m_stdin = nullptr;
    GInputStream* m_stdout = nullptr;
    GInputStream* m_stderr = nullptr;

    OutputCallback m_stdoutCallback;
    OutputCallback m_stderrCallback;
    ExitCallback m_exitCallback;

    std::atomic<bool> m_running{false};
    std::mutex m_mutex;
    
    // Buffer for reading
    char m_stdoutBuffer[4096];
    char m_stderrBuffer[4096];
};

}
