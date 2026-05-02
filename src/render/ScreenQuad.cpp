#include "ScreenQuad.h"

namespace se::render {

void ScreenQuad::draw() const {
    m_Vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

}  // namespace se::render
