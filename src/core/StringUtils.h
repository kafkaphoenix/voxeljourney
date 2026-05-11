#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <ranges>
#include <string_view>
#include <type_traits>

namespace se::core::str {

// -------------------------
// Character helpers
// -------------------------

inline bool isWhitespace(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }

inline bool isAllWhitespace(std::string_view s) {
    return std::ranges::all_of(s, [](unsigned char c) { return std::isspace(c) != 0; });
}

// -------------------------
// ASCII helpers
// -------------------------

inline char toLowerAscii(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c; }

// -------------------------
// Case-insensitive compare
// -------------------------

inline bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
    }

    return true;
}

// -------------------------
// Trim (no allocation)
// -------------------------

inline std::string_view trim(std::string_view s) {
    size_t start = 0;
    while (start < s.size() && isWhitespace(s[start])) { ++start; }

    size_t end = s.size();
    while (end > start && isWhitespace(s[end - 1])) { --end; }

    return s.substr(start, end - start);
}

// -------------------------
// Number parsing
// -------------------------

template <typename T>
std::optional<T> parseNumber(std::string_view s) {
    static_assert(std::is_arithmetic_v<T>);

    T value{};
    const char* begin = s.data();
    const char* end = begin + s.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);

    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }

    return value;
}

// -------------------------
// Bool parsing
// -------------------------

inline std::optional<bool> parseBool(std::string_view s) {
    s = trim(s);

    if (iequals(s, "1") || iequals(s, "true") || iequals(s, "yes") || iequals(s, "on")) {
        return true;
    }

    if (iequals(s, "0") || iequals(s, "false") || iequals(s, "no") || iequals(s, "off")) {
        return false;
    }

    return std::nullopt;
}

// -------------------------
// Prefix / suffix
// -------------------------

inline bool startsWith(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

inline bool endsWith(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

// -------------------------
// SplitView
// -------------------------

class SplitView {
public:
    class Iterator {
    public:
        Iterator(std::string_view str, char delim, size_t pos) : m_Str(str), m_Delim(delim), m_Pos(pos) {}

        std::string_view operator*() const {
            size_t next = m_Str.find(m_Delim, m_Pos);
            return m_Str.substr(m_Pos, next - m_Pos);
        }

        Iterator& operator++() {
            size_t next = m_Str.find(m_Delim, m_Pos);
            if (next == std::string_view::npos) {
                m_Pos = m_Str.size();
            } else {
                m_Pos = next + 1;
            }
            return *this;
        }

        bool operator!=(const Iterator& other) const { return m_Pos != other.m_Pos; }

    private:
        std::string_view m_Str;
        char m_Delim;
        size_t m_Pos;
    };

    SplitView(std::string_view str, char delim) : m_Str(str), m_Delim(delim) {}

    [[nodiscard]] Iterator begin() const { return {m_Str, m_Delim, 0}; }
    [[nodiscard]] Iterator end() const { return {m_Str, m_Delim, m_Str.size()}; }

private:
    std::string_view m_Str;
    char m_Delim;
};

}  // namespace se::core::str