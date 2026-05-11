# Voxel Journey
This project is a fork of [simpleengine](https://github.com/kafkaphoenix/simpleengine), focused on building a voxel engine. It’s fully open-source and documented as it evolves.
Along the way, I’ll be sharing blog posts that cover design decisions, implementation details, and the challenges I run into. You can find them in the blog section [voxel-journey](https://kafkaphoenix.github.io/categories/voxel-journey/).

## Engine Features
- OpenGL instanced forward renderer supporting CPU batching by mesh/material, and frustum culling.
- Models with optional support for skeletal animation.
- Diffuse lighting with multiple light sources.
- Simple camera controller with mouse look and WASD movement.
- Basic stats display with configurable update interval.
- Off-screen framebuffer with HDR (RGBA16F) and post-processing pipeline (tone mapping, inversion, grayscale, sharpen, blur, edge detect).
- Simple event system for input handling and window events.
- Asset manager with caching using lightweight handles.
- Simple config system for runtime settings.

For a more detailed look at the engine’s architecture, check out the [game-engine](https://kafkaphoenix.github.io/categories/game-engine/) blog section and the [architecture](docs/architecture.md).

## Build & Run

See the [build instructions](docs/build.md) for detailed steps on how to build and run the engine.

## Controls
- WASD: Move
- Mouse: Look
- Space / Left Ctrl: Up / down
- F3: Wireframe toggle
- F4: Cycle post-process effect
- F12: Toggle fullscreen
- Esc: Quit
