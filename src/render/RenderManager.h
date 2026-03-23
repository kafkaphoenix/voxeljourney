#pragma once
#include <optional>

#include "Frustum.h"
#include "ModelRenderer.h"
#include "RenderStats.h"

namespace se::scene {
class Camera;
class LightData;
class Renderable;
}

namespace se::render {

class RenderManager {
   public:
    RenderManager();

    void beginFrame(const se::scene::Camera& camera);
    void submit(const se::scene::Renderable& renderable);
    void endFrame(const se::scene::LightData& lights);

    void toggleWireframe();
    void setBatchSize(size_t maxInstances);
    void reset();

    const RenderStats& getStats() const noexcept;

   private:
    void clear();
    void setupGlState();

    const se::scene::Camera* m_Camera = nullptr;
    Frustum m_Frustum{};
    ModelRenderer m_ModelRenderer;
    RenderStats m_Stats;
    bool m_Wireframe = false;
};

}  // namespace se::render