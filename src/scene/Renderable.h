#pragma once
#include "Transform.h"
#include "assets/Material.h"

namespace se::render {
class Mesh;
}

namespace se::scene {

struct Renderable {
    se::render::Mesh* mesh = nullptr;
    se::assets::MaterialHandle material;
    Transform transform;
};

}  // namespace se::scene
