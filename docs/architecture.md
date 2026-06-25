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
- Input: Frame-based input state built from events; sampled once per render frame before simulation.
- Event: Base class for events, with derived classes for specific event types (keyboard, mouse, window, framebuffer resize)
- EventBus: Dispatches events to registered listeners.
- Config: Loads and stores engine configuration from a TOML file.
- Level: Owns the fixed-step gameplay loop, samples player intent, and advances player, scene animation, and root motion on a shared 60 Hz tick.
- StatsTracker: Tracks and displays engine stats like FPS, frame time, and draw calls.
- MemoryUtils: Used for memory tracking.
- Timer: Provides high-resolution timing for frame time and delta time calculations.

### Assets
- Asset: Base class for all asset types (shader, texture, model, material, etc).
- UUID: Generates unique identifiers.
- StringHash: Generates a hash from a string_view for fast lookups and comparisons.
- AssetHandle: Lightweight handle to an asset, used for referencing assets without owning them.
- AssetManager: Manages loading, caching, and unloading of assets. Provides a unified interface for asset access.
- Shader: Loads GLSL shader source code, compiles and links into an OpenGL program.
- Texture: Loads image data into an OpenGL texture using DSA.
- Material: Defines shader, textures, and render states for rendering a mesh.
- Model: Loads glTF/glb into meshes and materials, generates missing normals/tangents, and normalizes skinning weights when needed.
- Animation: Stores skeletal clips/channels/keyframes and samples a clip into a local-space pose.
- Skeleton: Defines bone hierarchy, inverse bind matrices, and rest pose; builds final skinning palette matrices from a sampled pose.

### Render
- VertexArray: OpenGL VAO wrapper for vertex attribute setup.
- BufferLayout: Describes vertex attribute formats and offsets for VAO setup.
- Buffer: OpenGL buffer wrapper for vertex/index data.
- Mesh: Vertex/index data loaded from models, with OpenGL buffers and VAO setup.
- UniformBuffer: OpenGL UBO wrapper for per-frame data (camera, lights).
- Frustum: Simple CPU frustum culling for renderables outside the camera view.
- ModelSubmission: Represents a single draw call for a mesh with its associated material, transform, model matrix, and optional bone matrices.
- RenderManager: Orchestrates passes (geometry, skybox, post-process), framebuffer management, and MSAA resolve.
- RenderStats: Tracks draw calls, triangles for stats display.
- VisibilityMask: Defines visibility layers for renderables and cameras, allowing selective rendering of objects based on their assigned layer.
- Framebuffer: Off-screen render target with color/depth attachments. Supports single-sample, MSAA, and HDR configurations.
- FrameRenderData: Stores per-frame data like camera matrices, lights, and other global parameters for rendering.
- SceneRenderAdapters: Adapts scene objects (renderables, lights, cameras) into renderable data structures for the renderer.
- UboDefinitions: Defines the layout of uniform buffer objects (UBOs) for frames, lights, and other global data used in shaders.
- ModelRenderer: Submits renderables to the RenderQueue and renders opaque and transparent passes for static and animated items.
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
- Player: Owns player-specific gameplay orchestration, including camera mode, locomotion animation state, intent sampling, and body sync.
- PlayerIntent: Stores gameplay intent produced once per render frame from the current `Input` state and consumed by fixed simulation steps.
- Transform: Defines position, rotation, and scale.
- Camera: Defines a camera with position, orientation, FOV, clip planes, and visibility mask for selective rendering.
- CameraController: Handles third/first-person camera state from sampled player intent and syncs the render camera to the player.
- Animator: Updates skeleton poses, extracts grounded root motion, and generates bone matrices for skeletal animation.
- AnimationController: High-level locomotion state controller driving Animator clip transitions.
- AnimatedInstance: Runtime scene object combining model, transform, animator, and tag.
- CharacterController: Converts player intent into facing, movement direction, and non-root-motion translation.

## Gameplay update flow

1. `Application::beginFrame` clears transient input state and dispatches window/input events.
2. `Level::update` reads the current `Input` state once, converts it into a `PlayerIntent`, and accumulates render-frame time.
3. While the accumulator contains at least one fixed step, `Level` advances the simulation in this order:
    - `Player::update` consumes `PlayerIntent` and updates controller/camera mode state.
    - `Scene::update` advances all `Animator` instances on the same fixed tick.
    - `Player::finalizeFrame` consumes root motion, syncs the camera, and syncs the player body transform.
4. Per-frame events in `PlayerIntent` such as mouse delta and camera toggles are cleared after the first simulation step that consumes them, while continuous state such as movement and running persists.

This avoids replaying edge-triggered input across multiple fixed substeps and keeps controller-authored movement, animation sampling, and root motion on the same timestep.

## Animation and root motion flow

Animated content flows through the engine in four stages:

1. `assets::Model` loads meshes, materials, skeletons, skins, and animation clips from glTF.
2. `assets::AnimationClip` stores channels and can sample a local pose for any clip time. It also identifies the best root-motion bone by preferring translated bones named like `root`, `hip`, or `pelvis` and then choosing the shallowest match.
3. `scene::Animator` advances clip time, samples the current pose, extracts root-motion delta on the X/Z plane, and builds the final skinning palette. When root motion is enabled, the same mask used for gameplay motion is applied when locking the animated root bone back toward its rest translation so the mesh does not visually double-move.
4. `scene::Player` decides how to use the extracted delta. If root motion is enabled, it applies the animator's delta in world space during `finalizeFrame`; if the clip displacement is too small relative to controller-authored fallback movement, it uses the fallback translation instead (defined by walk/run speeds and facing). The player body transform is then copied to the `AnimatedInstance` for rendering.

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
4. If MSAA is enabled: resolve scene color + depth from MSAA buffer to single-sample final buffer.
5. Transparent OIT pass (for materials tagged as OIT; useful for particle-like and intersecting transparent effects where strict sort order is hard).
6. Transparent depth-sorted pass (for materials tagged as sorted, and also the default fallback when no transparency tag is provided in glTF extras).
7. Post-process full-screen pass.

Note: OIT accum/reveal attachments are rendered on the single-sample final buffer (not MSAA-resolved) to avoid incorrect averaging artifacts.

## Potential improvements ideas
- Better error handling and logging. Using a logging library like spdlog would be a good improvement.
- More robust asset management with reference counting and unloading/reloading.
- Overhaul renderer (forward+ or deferred) and add more features like shadows and reflections or more complex lighting logic.
- More complete input handling with action mapping and support for gamepads.
- More complete scene management with entities, components, and systems (ECS architecture).
- State management for different game states (main menu, gameplay, pause, etc).
- Debug rendering and tools for inspecting the scene and assets. Using a library like ImGui would be great for this.
- UI system for in-game menus, HUD, etc. Using RmlUI or similar would be a good option.
- Multithreading for asset loading(it requires mutexes for their maps) and potentially rendering (if using Vulkan as OpenGL is not thread-friendly).
- Using Vulkan instead of OpenGL for better performance and modern features (like ray tracing, compute shaders, and explicit multi-threading), though it would require a significant rewrite of the renderer and shader system.
- Serialization for saving/loading scenes and assets.
- Editor mode with real-time scene editing and asset management.
- Memory and Performance profiling to identify bottlenecks and optimize critical paths. Using a profiler like Tracy would be very helpful for this.
- Event system improvements:
    1. Add a handled flag or priority to stop propagation (useful for UI capturing input).
    2. Add event categories to subscribe to groups (e.g., an input layer only listens to keyboard/mouse, editor tools only listen to window events).
    3. Keep deferred (queued) events but optionally add immediate dispatch for input-only events (keyboard, mouse).
- Multi-window support.