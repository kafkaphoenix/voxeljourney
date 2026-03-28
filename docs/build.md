# Build Instructions

This project uses **[vcpkg](https://github.com/microsoft/vcpkg)** for dependency management and **[CMake](https://cmake.org/) + [Make](https://www.gnu.org/software/make/)** for building.

> Supported platforms: Windows, Linux
> Requires: CMake ≥ 3.20, Make, C++23 compiler

> A CMakePresets.json file and launch configurations are included for easy configuration with VS Code's CMake Tools extension.

---

### 1. Install Dependencies
First install the required libraries using vcpkg:

On Windows:
```bash
vcpkg install --triplet x64-windows
```
or on Linux:
```bash
vcpkg install --triplet x64-linux
```
### 2. Configure
Configure the project with the correct toolchain:
```bash
make configure
```
> This automatically detects your OS and selects the appropriate vcpkg path.
### 3. Build
Build the project:
```bash
make build
```
### 4. Run
Run the demo:
```bash
make run
```
### 5. Clean Build Files
Optionally, you can clean the build files with:
```bash
make clean
```
### 6. Attach Renderdoc
To capture frames with Renderdoc, first launch the engine with:
```bash
make renderdoc
```
Then open Renderdoc, select the running process, and start capturing frames.
> Labels are used in the render logic to help identify draw calls and resources in Renderdoc.

### 7. Static Analysis
To run clang-tidy static analysis:
```bash
make tidy
```
> This requires CMake to be configured with `-DENABLE_CLANG_TIDY=ON` and a compatible clang-tidy installation. Make sure to have clang-tidy in your PATH for this to work. It uses the .clang-tidy file included in the project.

### 8. Code Formatting
To format the code using clang-format:
```bash
make format
```
> Make sure to have clang-format in your PATH for this to work. It uses the .clang-format file included in the project.