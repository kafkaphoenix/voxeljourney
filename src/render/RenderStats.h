#pragma once

namespace se::render {

struct RenderStats {
    unsigned int modelDrawCalls = 0;
    unsigned int modelTriangles = 0;
    unsigned int chunkDrawCalls = 0;
    unsigned int chunksVisible = 0;
    unsigned int chunkTriangles = 0;

    void reset() noexcept { modelDrawCalls = modelTriangles = chunkDrawCalls = chunksVisible = chunkTriangles = 0; }
};

}  // namespace se::render