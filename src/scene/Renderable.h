#pragma once

#include "assets/Material.h"
#include "Transform.h"

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
