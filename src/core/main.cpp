#include "Application.h"

#include <cstdio>
#include <exception>
#include <print>

int main() {
    try {
        se::core::Application app;
        app.run();
    } catch (const std::exception& e) {
        std::println(stderr, "An error occurred: {}", e.what());
        return 1;
    }
    return 0;
}
