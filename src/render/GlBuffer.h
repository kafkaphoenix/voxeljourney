#pragma once

#include <glad/glad.h>

namespace se::render {

class GlBuffer {
   public:
    explicit GlBuffer(GLenum target);
    ~GlBuffer();

    GlBuffer(const GlBuffer&) = delete;
    GlBuffer& operator=(const GlBuffer&) = delete;
    GlBuffer(GlBuffer&& other) noexcept;
    GlBuffer& operator=(GlBuffer&& other) noexcept;

    void setData(GLsizeiptr size, const void* data, GLenum usage) const;
    void updateSubData(GLintptr offset, GLsizeiptr size, const void* data) const;
    unsigned int id() const { return m_Id; }

   private:
    void release();

    GLuint m_Id = 0;
    GLenum m_Target;
};

}  // namespace se::render
