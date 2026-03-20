# Voxel Journey
This project is a fork of [simpleengine](https://github.com/kafkaphoenix/simpleengine), focused on building a voxel engine. It’s fully open-source and documented as it evolves.
Along the way, I’ll be sharing blog posts that cover design decisions, implementation details, and the challenges I run into. You can find them in the blog section [voxel-journey](https://kafkaphoenix.github.io/categories/voxel-journey/).

## Engine Features
- Instanced forward rendering, CPU batching by mesh/material with frustum culling.
- glTF/glb model loading with tinygltf.
- Frame UBO for per-frame camera and light data.
- Directional sun + ambient + optional point lights.
- Simple camera controller with mouse look and WASD movement.
- Wireframe toggle and fullscreen mode.
- Basic stats display with configurable update interval.
- Simple event system for input handling.
- Asset manager with caching for shaders, textures, materials and models using AssetHandle references.
- Simple config system with INI sections.

For a more detailed look at the engine’s architecture and possible improvements, check out the [game-engine](https://kafkaphoenix.github.io/categories/game-engine/) blog section and the [architecture](docs/architecture.md).

## Build & Run

See the [build instructions](docs/build.md) for detailed steps on how to build and run the engine.

## Controls
- WASD: Move
- Mouse: Look
- Space / Left Ctrl: Up / down
- F3: Wireframe toggle
- F12: Toggle fullscreen
- Esc: Quit
