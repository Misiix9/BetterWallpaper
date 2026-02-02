#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

// Forward declare PulseAudio types to avoid header pollution
typedef struct pa_simple pa_simple;

namespace bwp::audio {

class AudioAnalyzer {
public:
  static AudioAnalyzer &getInstance() {
    static AudioAnalyzer instance;
    return instance;
  }

  // Start/stop audio capture
  void start();
  void stop();
  bool isRunning() const { return m_running; }

  // Get current frequency band data (normalized 0-1)
  std::vector<float> getBands() const;
  
  // Number of frequency bands to compute
  static constexpr int NUM_BANDS = 16;
  
  // Callback when new audio data is processed
  using AudioCallback = std::function<void(const std::vector<float>&)>;
  void setCallback(AudioCallback cb) { m_callback = cb; }

private:
  AudioAnalyzer();
  ~AudioAnalyzer();
  
  void captureLoop();
  void computeFFT(const std::vector<float>& samples);
  
  // PulseAudio handle
  pa_simple* m_paHandle = nullptr;
  
  // Thread management
  std::thread m_captureThread;
  std::atomic<bool> m_running{false};
  std::atomic<bool> m_stopFlag{false};
  
  // Audio data
  mutable std::mutex m_mutex;
  std::vector<float> m_bands;
  
  // Callback
  AudioCallback m_callback;
  
  // Sample rate and buffer size
  static constexpr int SAMPLE_RATE = 44100;
  static constexpr int BUFFER_SIZE = 1024; // ~23ms at 44.1kHz
};

} // namespace bwp::audio
