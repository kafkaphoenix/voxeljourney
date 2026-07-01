# Engine Architecture

## Project layout

- assets: used for runtime assets like shaders, textures and models. These are loaded and copied to the build output by CMake.
- build: CMake build output.
- src: engine source code, organized into modules.
- CMakeLists.txt: CMake configuration for the project, including dependencies and build targets.
- .clang-format: ClangFormat configuration for code formatting.
- .clang-tidy: ClangTidy configuration for static analysis.
- Makefile: Makefile for build automation (check [build instructions](./build.md) for usage).
- vcpkg.json: vcpkg manifest for dependencies.
- vcpkg-configuration.json: vcpkg configuration for custom triplets and settings.
- CMakePresets.json: CMake presets for easy configuration with VS Code's CMake Tools extension.
- .vscode/launch.json: VS Code launch configurations for debugging.

## Project dependencies

- [OpenGL](https://www.opengl.org/) + [GLAD](https://github.com/dav1dde/glad) for rendering.
- [GLFW](https://www.glfw.org/) for creating windows and handling input.
- [GLM](https://github.com/g-truc/glm) for math (vectors, matrices, etc).
- [stb_image](https://github.com/nothings/stb/tree/master) for loading textures.
- [tinygltf](https://github.com/syoyo/tinygltf) for loading glTF models.
- [tomlplusplus](https://github.com/marzer/tomlplusplus) for configuration parsing.

## Modules

The engine is organized into several modules, each responsible for a specific aspect of the engine's functionality. Below is an overview of the main modules and their responsibilities.

### Core

- main: Minimal entry point that boots the application.
- Application: Owns the main loop and orchestrates the engine subsystems.
- Window: Sets up GLFW window and OpenGL context and handles window events.
- Input: Frame-based input state built from events. Sampled once per render frame before simulation and consumed by the fixed-step loop.
- Event: Base class for events, with derived classes for specific event types (keyboard, mouse, window, framebuffer resize)
- EventBus: Dispatches events to registered listeners.
- Config: Loads and stores engine configuration from a TOML file.
- Level: Owns the fixed-step simulation loop and orchestrates gameplay updates. It accumulates input into a PlayerIntent, advances player logic and scene animation on a 60 Hz tick, resolves root motion, and provides the render layer with the active camera and scene data each frame.
- StatsTracker: Tracks and periodically formats engine runtime statistics such as FPS, frame timing, memory usage, and render statistics for on-screen display or debugging overlays.
- MemoryUtils: Used for memory tracking.
- Timer: Provides high-resolution timing for frame time and delta time calculations.

### Assets

- Asset: Base class for all asset types.
- UUID: Generates unique identifiers.
- StringHash: Generates a hash from a string_view for fast lookups and comparisons.
- AssetHandle: Lightweight handle to an asset, used for referencing assets without owning them.
- AssetManager: Manages loading, caching, and unloading of assets. Provides a unified interface for asset access.
- Shader: Loads GLSL shader source code, compiles and links into an OpenGL program.
- Texture: Loads image data into an OpenGL texture using DSA.
- Material: Defines shader, textures, and render states for rendering a mesh.
- Model: Loads glTF/glb assets, including meshes, materials, skeletons, skins, and animation clips. Generates missing tangents, normalizes skinning weights, and creates the runtime asset representation.
- Animation: Stores skeletal animation clips, including channels and keyframes, and samples them into a local-space pose.
- Skeleton: Defines bone hierarchy, inverse bind poses, and rest pose, and builds the final skinning palette from a sampled pose.

### Render

- VertexArray: OpenGL VAO wrapper for vertex attribute setup.
- BufferLayout: Describes vertex attribute formats and offsets for VAO setup.
- Buffer: OpenGL buffer wrapper for vertex/index data.
- Mesh: Vertex/index data loaded from models, with OpenGL buffers and VAO setup.
- UniformBuffer: OpenGL UBO wrapper for per-frame data (camera, lights).
- Frustum: Simple CPU frustum culling for renderables outside the camera view.
- ModelSubmission: Represents a single draw call for a mesh, including its material, transform, model matrix, and optional skeletal bone palette for animated rendering.
- RenderManager: Orchestrates the full rendering pipeline, including render passes (geometry, skybox, post-processing), framebuffer management, MSAA resolve, and submission coordination.
- RenderStats: Tracks rendering statistics such as draw calls and triangle counts for both static and animated geometry, used for debugging and performance monitoring.
- VisibilityMask: Defines visibility layers for renderables and cameras, allowing selective rendering of objects based on their assigned layer.
- Framebuffer: Off-screen render target with color/depth attachments. Supports single-sample, MSAA, and HDR configurations.
- FrameRenderData: Contains per-frame render-facing data such as camera and lighting state. This data is generated each frame via scene render adapters and used by the renderer for shading and draw submission.
- SceneRenderAdapters: Converts scene objects (camera, lights, renderables) into render-facing data structures used by the renderer.
- UboDefinitions: Defines the structure of uniform buffers used for frame, lighting, and other global shader data.
- ModelRenderer: Performs frustum culling, submits renderables to the RenderQueue, batches compatible static draws, and renders opaque and transparent passes for both static and animated geometry.
- RenderQueue: Collects renderables each frame and classifies them into static opaque batches, animated opaque draws, OIT transparent draws, and depth-sorted transparent draws.
- TerrainRenderer: Submits visible chunk renderables to a draw list and handles terrain-specific rendering logic.
- PostProcessRenderer: Full-screen post-processing pass with selectable effects (tone map, inversion, grayscale, sharpen, blur, edge detect).
- ScreenQuad: Attributeless full-screen triangle for post-processing.

### Scene

- Scene: Owns the scene objects, lights, and the sky.
- SceneBuilder: Builds the initial scene with a sun, sky, player and the terrain.
- Light: Defines different light types (directional, point, and spot).
- LightData: Stores all light properties in a format suitable for uploading to a UBO.
- Sun: Directional light with color and intensity.
- Sky: Simple sky color and ambient light.
- Renderable: Defines a renderable mesh/material with either static pose data or dynamic transform/animator references.
- ChunkRenderable: Renderable for a chunk composed of a mesh and transform.
- Player: Coordinates player gameplay, including movement, camera state, animation, root motion, and body synchronization.
- PlayerIntent: Stores the player's intended actions for the current frame, sampled from Input and consumed during fixed simulation updates.
- Transform: Defines position, rotation, and scale.
- Camera: Defines a camera with position, orientation, FOV, clip planes, and a visibility mask for selective rendering.
- CameraController: Handles first and third-person camera behavior, including view rotation, mode switching, and follow logic based on player state.
- Animator: Updates skeletal poses, extracts grounded root motion, and generates bone matrices for skinning.
- AnimationController: Locomotion state machine that drives animation clip selection and transitions in the Animator.
- AnimatedInstance: Runtime scene object combining model, transform, animator, and tag.
- CharacterController: Converts player intent into facing, movement direction, and non-root-motion translation.

## Gameplay update flow

1. `Application::beginFrame` clears transient input state and dispatches window/input events.
2. `Level::update` samples Input once per frame, converts it into a `PlayerIntent`, and accumulates frame time for fixed-step simulation.
3. While the accumulator contains at least one fixed step, `Level` advances the simulation in order:
    - `Player::update` consumes `PlayerIntent` and updates movement, facing, and camera mode state
    - `Scene::update` advances all `Animator` instances on the fixed tick
    - `Player::finalizeFrame` applies root motion, and synchronizes camera and body transform
4. Edge-triggered inputs in `PlayerIntent` (e.g., mouse delta, camera toggle) are cleared after the first consumed step, while continuous inputs (movement, running) persist across steps.

This avoids reprocessing transient input across multiple substeps and keeps movement, animation sampling, and root motion tightly synchronized on the same simulation tick.

## Animation and root motion flow

Animated content flows through the engine in four stages:

1. `assets::Model` loads meshes, materials, skeletons, skins, and animation clips from glTF.
2. `assets::AnimationClip` stores channels and keyframes, and samples a local-space pose at any time. It also selects a root-motion bone, preferring bones named root, hip, or pelvis, and falling back to the shallowest match in the hierarchy.
3. `scene::Animator` advances clip time, samples the current pose, extracts root-motion delta on the X/Z plane, and builds the final skinning palette. When root motion is enabled, it constrains the animated root so motion is not double-applied between animation and gameplay.
4. `scene::Player` resolves final motion: it applies root motion in world space when enabled, otherwise falls back to controller-driven movement based on walk/run speeds and facing direction. The resulting transform is then applied to the `AnimatedInstance` for rendering.

### Voxel
- Voxel: Defines a voxel with a type (e.g., air, dirt, stone).
- Chunk: 16x16x16 voxels, stores voxel data and provides accessors.
- ChunkMap: Manages loaded chunks in a hashmap, provides access to voxel data and handles chunk loading/unloading.
- ChunkMesher: Generates a mesh for a chunk by checking visible faces and creating vertices accordingly.
- ChunkCoords: Utility functions for converting between world, chunk, and voxel coordinates.
- World: Owns the ChunkMap and handles chunk streaming based on the player's position. It also coordinates with the Scene to update chunk renderables when chunks are loaded or modified.

## Frame render order

1. Terrain pass.
2. Static opaque (instanced where possible, grouped by material/mesh to reduce state changes).
3. Animated opaque (non-instanced, per-draw bone palette update).
4. MSAA resolve (if enabled): scene color and depth are resolved to the single-sample final buffer
5. Transparent OIT pass (for materials tagged as OIT; used for intersecting or particle-like transparency where sorting is not feasible)
6. Transparent depth-sorted pass (for materials tagged as sorted, or as fallback when no transparency mode is specified in glTF extras)
7. Post-process full-screen pass.

Note: OIT accum/reveal buffers are written to the single-sample target (not the MSAA buffer) to avoid blending artifacts during resolve.

## Potential improvements ideas

- Better error handling and logging. Using a logging library like spdlog would be a good improvement.
- More robust asset management with reference counting and unloading/reloading.
- Evolve rendering pipeline toward forward+ or clustered lighting, with expanded shadowing and reflection support.
- More complete input handling with action mapping and support for gamepads.
- More complete scene management with entities, components, and systems (ECS architecture).
- State management for different game states (main menu, gameplay, pause, etc).
- Debug rendering and tools for inspecting the scene and assets. Using a library like ImGui would be great for this.
- UI system for in-game menus, HUD, etc. Using RmlUI or similar would be a good option.
- Multithreaded asset loading and streaming (render thread remains single-context, with possible future migration to Vulkan for broader parallelism)
- Potential future migration to Vulkan for explicit control, modern GPU features, and improved multithreaded submission, at the cost of a full renderer rewrite.
- Serialization for saving/loading scenes and assets.
- Editor mode with real-time scene editing and asset management.
- Memory and Performance profiling to identify bottlenecks and optimize critical paths. Using a profiler like Tracy would be very helpful for this.
- Event system improvements:
    1. Add a handled flag or priority to stop propagation (useful for UI capturing input).
    2. Add event categories to subscribe to groups (e.g., an input layer only listens to keyboard/mouse, editor tools only listen to window events).
    3. Keep deferred (queued) events but optionally add immediate dispatch for input-only events (keyboard, mouse).
- Multi-window support.
- Physics integration or character collision system (e.g. camera collision, environment interaction).