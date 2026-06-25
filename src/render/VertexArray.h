#pragma once

#include <glad/glad.h>

namespace se::render {

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void bind() const;
    [[nodiscard]] unsigned int id() const { return m_Id; }

    void enableAttrib(GLuint index) const;
    void setAttribFormat(GLuint index, GLint size, GLenum type, GLboolean normalized, GLuint relativeOffset) const;
    void setAttribIFormat(GLuint index, GLint size, GLenum type, GLuint relativeOffset) const;
    void setAttribBinding(GLuint index, GLuint binding) const;
    void setVertexBuffer(GLuint binding, GLuint buffer, GLintptr offset, GLsizei stride) const;
    void setElementBuffer(GLuint buffer) const;
    void setBindingDivisor(GLuint binding, GLuint divisor) const;

private:
    void release();

    unsigned int m_Id = 0;
};

}  // namespace se::render
