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

struct ProcessMemoryUsage {
    std::size_t usedKB = 0;
    std::size_t committedKB = 0;
};

inline ProcessMemoryUsage getProcessMemoryUsageKB() {
#if defined(_WIN32)

    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
            sizeof(pmc))) {
        ProcessMemoryUsage usage{};
        usage.usedKB = static_cast<std::size_t>(pmc.WorkingSetSize / 1024);
        usage.committedKB = static_cast<std::size_t>(pmc.PrivateUsage / 1024);
        return usage;
    }

    return {};

#else

    std::ifstream statm("/proc/self/statm");
    if (!statm.is_open())
        return {};

    std::size_t size = 0;
    std::size_t resident = 0;
    if (!(statm >> size >> resident))
        return {};

    const long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
    ProcessMemoryUsage usage{};
    usage.usedKB = resident * static_cast<std::size_t>(page_size_kb);
    usage.committedKB = size * static_cast<std::size_t>(page_size_kb);
    return usage;

#endif
}

}  // namespace se::core