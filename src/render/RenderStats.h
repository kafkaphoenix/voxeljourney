#pragma once

namespace se::render {

struct RenderStats {
    unsigned int modelDrawCalls = 0;
    unsigned int modelTriangles = 0;
    unsigned int chunksDrawCalls = 0;
    unsigned int chunksTriangles = 0;
    unsigned int animatedModelDrawCalls = 0;
    unsigned int animatedModelTriangles = 0;

    void reset() noexcept {
        modelDrawCalls = modelTriangles = animatedModelDrawCalls = animatedModelTriangles = chunksDrawCalls =
            chunksTriangles = 0;
    }
};

}  // namespace se::render