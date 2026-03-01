#pragma once

#include <chrono>

namespace se::core {

class Timer {
   public:
    Timer() { reset(); }

    void reset() { m_start = std::chrono::steady_clock::now(); }

    double get_seconds() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_start).count();
    }

    double get_milliseconds() const {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - m_start).count();
    }

   private:
    std::chrono::time_point<std::chrono::steady_clock> m_start;
};

}  // namespace se::core
