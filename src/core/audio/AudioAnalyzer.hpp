#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
typedef struct pa_simple pa_simple;
namespace bwp::audio {
class AudioAnalyzer {
public:
  static AudioAnalyzer &getInstance() {
    static AudioAnalyzer instance;
    return instance;
  }
  void start();
  void stop();
  bool isRunning() const { return m_running; }
  std::vector<float> getBands() const;
  static constexpr int NUM_BANDS = 16;
  using AudioCallback = std::function<void(const std::vector<float>&)>;
  void setCallback(AudioCallback cb) { m_callback = cb; }
private:
  AudioAnalyzer();
  ~AudioAnalyzer();
  void captureLoop();
  void computeFFT(const std::vector<float>& samples);
  pa_simple* m_paHandle = nullptr;
  std::thread m_captureThread;
  std::atomic<bool> m_running{false};
  std::atomic<bool> m_stopFlag{false};
  mutable std::mutex m_mutex;
  std::vector<float> m_bands;
  AudioCallback m_callback;
  static constexpr int SAMPLE_RATE = 44100;
  static constexpr int BUFFER_SIZE = 1024;  
};
}  
