#pragma once

namespace se::render {

struct RenderStats {
    unsigned int modelDrawCalls = 0;
    unsigned int modelTriangles = 0;

    void reset() noexcept { modelDrawCalls = modelTriangles = 0; }
};

}  // namespace se::render