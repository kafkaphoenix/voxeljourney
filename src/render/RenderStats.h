#pragma once

namespace se::render {

struct RenderStats {
    unsigned int modelsDrawCalls = 0;
    unsigned int modelsTriangles = 0;
    unsigned int chunksDrawCalls = 0;
    unsigned int chunksTriangles = 0;
    unsigned int animatedModelsDrawCalls = 0;
    unsigned int animatedModelsTriangles = 0;

    void reset() noexcept {
        modelsDrawCalls = modelsTriangles = animatedModelsDrawCalls = animatedModelsTriangles = chunksDrawCalls =
            chunksTriangles = 0;
    }
};

}  // namespace se::render