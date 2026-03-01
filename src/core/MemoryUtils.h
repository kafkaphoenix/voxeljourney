#include <cstddef>
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <fstream>
#endif

namespace se::core {

inline std::size_t getProcessMemoryUsageKB() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
    return 0;
#else
    std::ifstream statm("/proc/self/statm");
    if (!statm.is_open()) return 0;

    std::size_t size = 0, resident = 0;
    if (!(statm >> size >> resident)) return 0;

    const long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
    return resident * page_size_kb;
#endif
}

}  // namespace se::core