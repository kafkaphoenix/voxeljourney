#include <cstddef>
#include <string>

#if defined(_WIN32)

#include <psapi.h>
#include <windows.h>

#else

#include <unistd.h>

#include <fstream>
#include <sstream>

#endif

namespace se::core {

inline std::size_t getProcessMemoryUsageKB() {
#if defined(_WIN32)

    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
            sizeof(pmc))) {
        return static_cast<std::size_t>(pmc.PrivateUsage / 1024);
    }

    return 0;

#else

    std::ifstream file("/proc/self/smaps_rollup");
    if (!file.is_open())
        file.open("/proc/self/smaps");  // older kernel fallback but more expensive to parse
    if (!file.is_open())
        return 0;

    std::string line;
    std::size_t private_kb = 0;

    while (std::getline(file, line)) {
        if (line.rfind("Private_Clean:", 0) == 0 ||
            line.rfind("Private_Dirty:", 0) == 0) {
            std::istringstream iss(line);
            std::string key;
            std::size_t value;
            std::string unit;

            if (iss >> key >> value >> unit) {
                private_kb += value;
            }
        }
    }

    return private_kb;

#endif
}

}  // namespace se::core