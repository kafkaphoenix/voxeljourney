# Engine Architecture

## Project layout
- assets: Shaders, textures, models, and materials.
- build: CMake build output.
- src: Engine code.
- CMakeLists.txt, Makefile, vcpkg.json, vcpkg-configuration.json, CMakePresets.json, and launch configurations.

## Project dependencies
- OpenGL + GLAD for rendering.
- GLFW for windowing and input.
- GLM for math.
- stb_image for textures.
- tinygltf for glTF models.
- SimpleIni for config parsing.

## Modules
The engine is organized into several modules, each responsible for a specific aspect of the engine's functionality. Below is an overview of the main modules and their responsibilities.

### Core
- Application: Owns the main loop, window, render manager, asset manager, and scene.
- Window: GLFW setup, OpenGL context, and event callbacks.
- Input: Frame-based input state built from events.
- EventBus: Small event queue used by window callbacks.
- Config: Reads config.ini for runtime settings.
- StatsTracker: Tracks and averages frame time, draw calls, etc.
- Level: Owns the scene and the player. Loads the demo scene and updates the scene each frame. It also handles input for the player and the initial configuration of the scene.

### Assets
- Asset: Minimal base class with a path.
- AssetHandle: Lightweight, type-safe references to assets.
- AssetManager: Loads and caches shaders, textures, models, and materials.
- Shader: GLSL program compilation and uniform updates.
- Texture: Image loading and OpenGL texture setup.
- Material: Shader + textures + render state, matching glTF data. Lighting is simple diffuse.
- Model: Loads glTF/glb into meshes and materials.

### Render
- VertexArray: OpenGL VAO wrapper for vertex attribute setup.
- BufferLayout: Describes vertex attribute formats and offsets for VAO setup.
- Buffer: OpenGL buffer wrapper for vertex/index data.
- Mesh: Vertex/index data loaded from models, with OpenGL buffers and VAO setup.
- UniformBuffer: OpenGL UBO wrapper for per-frame data (camera, lights).
- Frustum: Simple CPU frustum culling for renderables outside the camera view.
- RenderManager: Manages rendering each frame, including frustum culling and calling renderers.
- ModelRenderer: Handles submitting model renderables to the RenderQueue and grouping them for efficient rendering.
- RenderQueue: Collects renderables each frame to be processed by each renderer at the end of the frame.
- RenderStats: Tracks draw calls, triangles for stats display.

### Scene
- Scene: Owns the scene objects, lights (sun), and the sky.
- SceneBuilder: Builds the demo scene with the Sponza model and sets up the lights.
- Light: Defines different light types (directional, point, and spot).
- Sun: Directional light with color and intensity.
- Sky: Simple sky color and ambient light.
- Transform: Defines position, rotation, and scale.
- Renderable: Defines a renderable object composed of a mesh, material, and transform.
- Camera: Simple perspective camera with view/projection matrix calculation.
- Player: Camera controller with WASD movement and mouse look, no physics, collisions or model.

## Potential improvements
- More complete glTF/glb support (animations, PBR materials, Draco compression, etc).
- Better error handling and logging. Using a logging library like spdlog would be a good improvement.
- More robust asset management with reference counting and unloading/reloading.
- Improve renderer (forward+ or deferred) and add more features like shadows, reflections, and post-processing.
- Adding more complex lighting models, shadows, and post-processing effects.
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
- Cube map support for skyboxes and reflections.
- Event system improvements:
    1. Add a handled flag or priority to stop propagation (useful for UI capturing input).
    2. Add event categories to subscribe to groups (e.g., an input layer only listens to keyboard/mouse, editor tools only listen to window events).
    3. Keep deferred (queued) events but optionally add immediate dispatch for input-only events (keyboard, mouse).
- Multi-window support.