#pragma once

namespace se::render {

struct RenderStats {
    unsigned int modelDrawCalls = 0;
    unsigned int modelTriangles = 0;
    unsigned int chunkDrawCalls = 0;
    unsigned int chunksVisible = 0;
    unsigned int chunkTriangles = 0;
    unsigned int animatedModelDrawCalls = 0;
    unsigned int animatedModelTriangles = 0;

    void reset() noexcept { modelDrawCalls = modelTriangles = animatedModelDrawCalls = animatedModelTriangles = chunkDrawCalls = chunksVisible = chunkTriangles = 0; }
};

}  // namespace se::render