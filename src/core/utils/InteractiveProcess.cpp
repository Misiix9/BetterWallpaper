#include "InteractiveProcess.hpp"
#include "Logger.hpp"
#include <iostream>

namespace bwp::utils {

InteractiveProcess::InteractiveProcess() {}

InteractiveProcess::~InteractiveProcess() {
    stop();
    g_clear_object(&m_subprocess);
}

bool InteractiveProcess::start(const std::vector<std::string>& args) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running) return false;

    GError* error = nullptr;
    GPtrArray* argv = g_ptr_array_new();
    for (const auto& arg : args) {
        g_ptr_array_add(argv, (gpointer)arg.c_str());
    }
    g_ptr_array_add(argv, nullptr);

    m_subprocess = g_subprocess_newv(
        (const gchar* const*)argv->pdata,
        (GSubprocessFlags)(G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE),
        &error
    );

    g_ptr_array_free(argv, TRUE);

    if (error) {
        LOG_ERROR("Failed to start process: " + std::string(error->message));
        g_error_free(error);
        return false;
    }

    m_stdin = g_subprocess_get_stdin_pipe(m_subprocess);
    m_stdout = g_subprocess_get_stdout_pipe(m_subprocess);
    m_stderr = g_subprocess_get_stderr_pipe(m_subprocess);

    m_running = true;

    // Start reading loops
    g_input_stream_read_async(m_stdout, m_stdoutBuffer, sizeof(m_stdoutBuffer) - 1, 
                              G_PRIORITY_DEFAULT, nullptr, onReadStdout, this);
    g_input_stream_read_async(m_stderr, m_stderrBuffer, sizeof(m_stderrBuffer) - 1, 
                              G_PRIORITY_DEFAULT, nullptr, onReadStderr, this);

    // Watch for exit
    g_subprocess_wait_async(m_subprocess, nullptr, onProcessExited, this);

    LOG_DEBUG("InteractiveProcess started: " + args[0]);
    return true;
}

void InteractiveProcess::writeLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running || !m_stdin) return;

    std::string data = line + "\n";
    GError* error = nullptr;
    if (!g_output_stream_write_all(m_stdin, data.c_str(), data.size(), nullptr, nullptr, &error)) {
        LOG_ERROR("Failed to write to stdin: " + std::string(error->message));
        g_error_free(error);
    } else {
        g_output_stream_flush(m_stdin, nullptr, nullptr);
    }
}

void InteractiveProcess::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running || !m_subprocess) return;
    
    g_subprocess_send_signal(m_subprocess, SIGTERM);
    m_running = false;
    g_clear_object(&m_subprocess);
    m_stdin = nullptr;
    m_stdout = nullptr;
    m_stderr = nullptr;
}

void InteractiveProcess::kill() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running || !m_subprocess) return;

    g_subprocess_force_exit(m_subprocess);
    m_running = false;
    g_clear_object(&m_subprocess);
    m_stdin = nullptr;
    m_stdout = nullptr;
    m_stderr = nullptr;
}

bool InteractiveProcess::isRunning() const {
    return m_running;
}

void InteractiveProcess::setStdoutCallback(OutputCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stdoutCallback = callback;
}
void InteractiveProcess::setStderrCallback(OutputCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stderrCallback = callback;
}
void InteractiveProcess::setExitCallback(ExitCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_exitCallback = callback;
}

void InteractiveProcess::onReadStdout(GObject* source, GAsyncResult* res, gpointer user_data) {
    auto* self = static_cast<InteractiveProcess*>(user_data);
    self->handleRead(G_INPUT_STREAM(source), res, self->m_stdoutCallback);
    
    // Continue reading if still running
    if (self->m_running) {
         g_input_stream_read_async(self->m_stdout, self->m_stdoutBuffer, sizeof(self->m_stdoutBuffer) - 1,
                                   G_PRIORITY_DEFAULT, nullptr, onReadStdout, self);
    }
}

void InteractiveProcess::onReadStderr(GObject* source, GAsyncResult* res, gpointer user_data) {
    auto* self = static_cast<InteractiveProcess*>(user_data);
    self->handleRead(G_INPUT_STREAM(source), res, self->m_stderrCallback);

    if (self->m_running) {
         g_input_stream_read_async(self->m_stderr, self->m_stderrBuffer, sizeof(self->m_stderrBuffer) - 1,
                                   G_PRIORITY_DEFAULT, nullptr, onReadStderr, self);
    }
}

void InteractiveProcess::handleRead(GInputStream* stream, GAsyncResult* res, OutputCallback& callback) {
    GError* error = nullptr;
    char* buffer = (stream == m_stdout) ? m_stdoutBuffer : m_stderrBuffer;
    gssize len = g_input_stream_read_finish(stream, res, &error);

    if (len > 0) {
        buffer[len] = '\0';
        std::string chunk(buffer);
        // Invoke callback safely? usually GAsync callbacks run in main loop (GTK/GLib)
        // Check if we need to dispatch?? 
        // For bwp::core logic, it should be fine.
        if (callback) {
            callback(chunk);
        }
    } else if (error) {
        // LOG_ERROR("Read error: " + std::string(error->message));
        g_error_free(error);
    }
}

void InteractiveProcess::onProcessExited(GObject* source, GAsyncResult* res, gpointer user_data) {
    auto* self = static_cast<InteractiveProcess*>(user_data);
    GError* error = nullptr;
    
    if (g_subprocess_wait_finish(G_SUBPROCESS(source), res, &error)) {
        int exitCode = g_subprocess_get_status(self->m_subprocess);
        self->m_running = false;
        if (self->m_exitCallback) {
            self->m_exitCallback(exitCode);
        }
    } else {
        if (error) g_error_free(error);
    }
    
    // cleanup
    g_clear_object(&self->m_subprocess);
    self->m_stdin = nullptr;
    self->m_stdout = nullptr;
    self->m_stderr = nullptr;
}

}
