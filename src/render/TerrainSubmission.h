#pragma once

namespace se::render {

class Mesh;

struct TerrainSubmission {
    const Mesh* mesh = nullptr;
};

}  // namespace se::render