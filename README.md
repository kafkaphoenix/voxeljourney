# Voxel Journey
This project is a fork of [simpleengine](https://github.com/kafkaphoenix/simpleengine), focused on building a voxel engine. It’s fully open-source and documented as it evolves.
Along the way, I’ll be sharing blog posts that cover design decisions, implementation details, and the challenges I run into. You can find them in the blog section [voxel-journey](https://kafkaphoenix.github.io/categories/voxel-journey/).

## Engine Features
- OpenGL forward renderer with static instancing (mesh/material batching), animated model support, and frustum culling.
- Models with optional support for skeletal animation.
- Diffuse lighting with multiple light sources.
- Third-person player controller with mouse look and character locomotion state.
- Basic stats display with configurable update interval.
- Off-screen HDR framebuffer pipeline with MSAA resolve and post-processing (tone map, inversion, grayscale, sharpen, blur, edge detect).
- Simple event system for input handling and window events.
- Asset manager with caching using lightweight handles.
- Simple config system for runtime settings.

For a more detailed look at the engine’s architecture, check out the [game-engine](https://kafkaphoenix.github.io/categories/game-engine/) blog section and the [architecture](docs/architecture.md).

## Build & Run

See the [build instructions](docs/build.md) for detailed steps on how to build and run the engine.

## Controls
- WASD: Move
- Left Shift: Run
- Mouse: Look
- Space / Left Ctrl: Up / down
- F3: Wireframe toggle
- F4: Cycle post-process effect
- F12: Toggle fullscreen
- Esc: Quit

## Runtime Config

Settings live in `config.toml` and are grouped by subsystem:

- `window`: title, size, position, vsync, mode
- `player`: spawn position
- `characterController`: walk/run speeds, mouse sensitivity/smoothing, fixed-step options
- `camera`: projection settings (FOV, near/far, aspect)
- `thirdPersonCameraController`: follow distance and height
- `render`: MSAA sample count and anisotropy level
- `postProcess`: exposure
- `stats`: enable flag and update interval
