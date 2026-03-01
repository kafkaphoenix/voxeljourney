#include "Application.h"

#include <exception>
#include <iostream>

int main() {
    try {
        se::core::Application app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
