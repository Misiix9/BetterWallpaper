#include "AudioAnalyzer.hpp"
#include "../utils/Logger.hpp"
#include <cmath>
#include <complex>
#include <pulse/simple.h>
#include <pulse/error.h>

namespace bwp::audio {

AudioAnalyzer::AudioAnalyzer() {
  m_bands.resize(NUM_BANDS, 0.0f);
}

AudioAnalyzer::~AudioAnalyzer() {
  stop();
}

void AudioAnalyzer::start() {
  if (m_running) return;
  
  // Set up PulseAudio
  pa_sample_spec ss;
  ss.format = PA_SAMPLE_FLOAT32LE;
  ss.rate = SAMPLE_RATE;
  ss.channels = 1; // Mono for simplicity
  
  int error;
  // Use monitor source to capture system audio output
  // nullptr for server (default), "audio capture" as stream name
  // We use PA_STREAM_RECORD to capture the monitor
  m_paHandle = pa_simple_new(
    nullptr,                // Default server
    "BetterWallpaper",      // Application name
    PA_STREAM_RECORD,       // Record (capture)
    nullptr,                // Default device (will use monitor if available)
    "Audio Visualizer",     // Stream description
    &ss,                    // Sample spec
    nullptr,                // Default channel map
    nullptr,                // Default buffering attributes
    &error
  );
  
  if (!m_paHandle) {
    LOG_ERROR("AudioAnalyzer: Failed to connect to PulseAudio: " + std::string(pa_strerror(error)));
    return;
  }
  
  LOG_INFO("AudioAnalyzer: Connected to PulseAudio");
  
  m_running = true;
  m_stopFlag = false;
  m_captureThread = std::thread(&AudioAnalyzer::captureLoop, this);
}

void AudioAnalyzer::stop() {
  if (!m_running) return;
  
  m_stopFlag = true;
  
  if (m_captureThread.joinable()) {
    m_captureThread.join();
  }
  
  if (m_paHandle) {
    pa_simple_free(m_paHandle);
    m_paHandle = nullptr;
  }
  
  m_running = false;
  LOG_INFO("AudioAnalyzer: Stopped");
}

std::vector<float> AudioAnalyzer::getBands() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_bands;
}

void AudioAnalyzer::captureLoop() {
  std::vector<float> buffer(BUFFER_SIZE);
  int error;
  
  while (!m_stopFlag) {
    // Read audio samples
    if (pa_simple_read(m_paHandle, buffer.data(), buffer.size() * sizeof(float), &error) < 0) {
      LOG_WARN("AudioAnalyzer: Read error: " + std::string(pa_strerror(error)));
      continue;
    }
    
    // Process FFT
    computeFFT(buffer);
    
    // Notify callback
    if (m_callback) {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_callback(m_bands);
    }
  }
}

void AudioAnalyzer::computeFFT(const std::vector<float>& samples) {
  // Simple DFT implementation (for small buffer sizes this is acceptable)
  // For production, use FFTW3 or similar
  
  const int N = samples.size();
  const int halfN = N / 2;
  
  // Compute magnitude spectrum for first half (positive frequencies)
  std::vector<float> magnitudes(halfN);
  
  for (int k = 0; k < halfN; ++k) {
    float real = 0.0f, imag = 0.0f;
    for (int n = 0; n < N; ++n) {
      float angle = 2.0f * M_PI * k * n / N;
      real += samples[n] * std::cos(angle);
      imag -= samples[n] * std::sin(angle);
    }
    magnitudes[k] = std::sqrt(real * real + imag * imag) / N;
  }
  
  // Average into NUM_BANDS frequency bands
  // Use logarithmic scaling for more perceptually accurate bands
  std::lock_guard<std::mutex> lock(m_mutex);
  
  for (int band = 0; band < NUM_BANDS; ++band) {
    // Logarithmic band edges
    float bandStart = std::pow(2.0f, band * std::log2(halfN) / NUM_BANDS);
    float bandEnd = std::pow(2.0f, (band + 1) * std::log2(halfN) / NUM_BANDS);
    
    int startIdx = std::max(0, static_cast<int>(bandStart));
    int endIdx = std::min(halfN - 1, static_cast<int>(bandEnd));
    
    float sum = 0.0f;
    int count = 0;
    for (int i = startIdx; i <= endIdx; ++i) {
      sum += magnitudes[i];
      count++;
    }
    
    float avg = (count > 0) ? sum / count : 0.0f;
    
    // Normalize and apply some smoothing
    // Scale factor is empirical - adjust based on typical audio levels
    float normalized = std::min(1.0f, avg * 10.0f);
    
    // Simple exponential smoothing for visual stability
    m_bands[band] = m_bands[band] * 0.7f + normalized * 0.3f;
  }
}

} // namespace bwp::audio
