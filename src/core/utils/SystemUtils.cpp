#include "SystemUtils.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <psapi.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace bwp::utils {

uint64_t SystemUtils::getProcessRSS() {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
    return pmc.WorkingSetSize;
  }
  return 0;
#else
  std::ifstream f("/proc/self/statm");
  if (f.is_open()) {
    long size, resident; // pages
    f >> size >> resident;
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize > 0 && resident > 0) {
      return (uint64_t)resident * pageSize;
    }
  }
  return 0;
#endif
}

std::string SystemUtils::formatBytes(uint64_t bytes) {
  const char *suffixes[] = {"B", "KB", "MB", "GB", "TB"};
  int suffixIndex = 0;
  double v = static_cast<double>(bytes);

  while (v >= 1024.0 && suffixIndex < 4) {
    v /= 1024.0;
    suffixIndex++;
  }

  std::stringstream ss;
  ss << std::fixed << std::setprecision(1) << v << " " << suffixes[suffixIndex];
  return ss.str();
}

int SystemUtils::runCommand(const std::string &cmd) {
  return std::system(cmd.c_str());
}

} // namespace bwp::utils
