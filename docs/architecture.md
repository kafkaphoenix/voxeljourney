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
- Application: Owns the main loop, window, render manager, asset manager, and scene.
- Window: GLFW setup, OpenGL context, and event callbacks.
- Input: Frame-based input state built from events.
- EventBus: Small event queue used by window callbacks.
- Config: Reads config.toml for runtime settings.
- StatsTracker: Tracks and averages frame time, draw calls, etc.
- Level: Owns and coordinates the core simulation objects: Scene, World, and Player. Each frame it drives chunk streaming and mesh rebuilding via World, forwards input to Player, and keeps Scene in sync with the current world state. Also responsible for initial scene setup on load.

### Assets
- Asset: Minimal base class with a path.
- AssetHandle: Lightweight, type-safe references to assets.
- AssetManager: Loads and caches shaders, textures, models, and materials.
- Shader: GLSL program compilation and uniform updates.
- Texture: Image loading and OpenGL texture setup.
- Material: Shader + textures + render state, matching glTF data. Lighting is simple diffuse.
- Model: Loads glTF/glb into meshes and materials.
- Animation: Stores skeletal animation clips, channels, and keyframes for animating bones over time.
- Skeleton: Defines the bone hierarchy, inverse bind matrices, and rest pose transforms used for skinning and animation.

### Render
- VertexArray: OpenGL VAO wrapper for vertex attribute setup.
- BufferLayout: Describes vertex attribute formats and offsets for VAO setup.
- Buffer: OpenGL buffer wrapper for vertex/index data.
- Mesh: Vertex/index data loaded from models, with OpenGL buffers and VAO setup.
- UniformBuffer: OpenGL UBO wrapper for per-frame data (camera, lights).
- Framebuffer: Off-screen render target with color textures (HDR) and depth. Supports multiple render targets and depth-only FBOs.
- Frustum: Simple CPU frustum culling for renderables outside the camera view.
- RenderManager: Orchestrates render passes (geometry, skybox, post-process) and framebuffer management.
- ModelRenderer: Handles submitting model renderables to the RenderQueue and grouping them for efficient rendering.
- TerrainRenderer: Submits visible chunk renderables to a draw list.
- PostProcessRenderer: Full-screen post-processing pass with selectable effects (tone map, inversion, grayscale, sharpen, blur, edge detect).
- ScreenQuad: Attributeless full-screen triangle for post-processing.
- RenderQueue: Collects renderables each frame to be processed by each renderer at the end of the frame.
- RenderStats: Tracks draw calls, triangles for stats display.

### Scene
- Scene: Owns the scene objects, lights, and the sky.
- SceneBuilder: Builds the initial scene with a sun, sky, player and the terrain.
- Light: Defines different light types (directional, point, and spot).
- Sun: Directional light with color and intensity.
- Sky: Simple sky color and ambient light.
- Transform: Defines position, rotation, and scale.
- Renderable: Defines a renderable object composed of a mesh, material, and transform.
- ChunkRenderable: Renderable for a chunk composed of a mesh and transform.
- Camera: Simple perspective camera with view/projection matrix calculation.
- Player: Camera controller with WASD movement and mouse look, no physics, collisions or model.

### Voxel
- Voxel: Defines a voxel with a type (e.g., air, dirt, stone).
- Chunk: 16x16x16 voxels, stores voxel data and provides accessors.
- ChunkMap: Manages loaded chunks in a hashmap, provides access to voxel data and handles chunk loading/unloading.
- ChunkMesher: Generates a mesh for a chunk by checking visible faces and creating vertices accordingly.
- ChunkCoords: Utility functions for converting between world, chunk, and voxel coordinates.
- World: Owns the ChunkMap and handles chunk streaming based on the player's position. It also coordinates with the Scene to update chunk renderables when chunks are loaded or modified.

## Potential improvements
- Better error handling and logging. Using a logging library like spdlog would be a good improvement.
- More robust asset management with reference counting and unloading/reloading.
- Improve renderer (forward+ or deferred) and add more features like shadows and reflections or more complex lighting logic.
- More complete input handling with action mapping and support for gamepads.
- More complete scene management with entities, components, and systems.
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