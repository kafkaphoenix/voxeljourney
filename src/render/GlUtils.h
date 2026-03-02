#pragma once

#include <glad/glad.h>

#include <format>
#include <stdexcept>
#include <string>

namespace se::render {

#ifndef NDEBUG
inline void checkGlError(const char* context) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        throw std::runtime_error(std::format("OpenGL error at {}: {}", context, err));
    }
}
#else
inline void checkGlError(const char* context) {
    (void)context;
}
#endif

}  // namespace se::render
