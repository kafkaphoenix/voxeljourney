#pragma once
#include <cstdint>
#include <limits>
#include <random>

namespace se::assets {

class UUID {
public:
    UUID() : m_uuid(dis(gen)) {}
    explicit UUID(uint64_t uuid) : m_uuid(uuid) {}

    explicit operator uint64_t() const { return m_uuid; }
    [[nodiscard]] uint64_t value() const { return m_uuid; }

    bool operator==(const UUID& other) const { return m_uuid == other.m_uuid; }
    bool operator!=(const UUID& other) const { return m_uuid != other.m_uuid; }
    bool operator<(const UUID& other) const { return m_uuid < other.m_uuid; }  // For ordered containers

private:
    static inline std::random_device rd{};
    // Use thread_local to ensure different sequences in different threads.
    static inline thread_local std::mt19937_64 gen{rd()};
    static inline std::uniform_int_distribution<uint64_t> dis{0, std::numeric_limits<uint64_t>::max()};

    uint64_t m_uuid{};
};

}  // namespace se::assets

namespace std {
template <>
struct hash<se::assets::UUID> {
    std::size_t operator()(const se::assets::UUID& uuid) const noexcept { return std::hash<uint64_t>{}(uuid.value()); }
};
}