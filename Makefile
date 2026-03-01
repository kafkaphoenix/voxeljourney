.DEFAULT_GOAL := help

BUILD_DIR := build
.DEFAULT_GOAL := help

VCPKG_DIR_LINUX := ~/vcpkg
VCPKG_DIR_WINDOWS := C:/vcpkg

ifeq ($(OS),Windows_NT)
BUILD_DIR := build/windows-msvc
VCPKG_DIR := $(VCPKG_DIR_WINDOWS)
EXE := $(BUILD_DIR)/Debug/simpleengine.exe
else
BUILD_DIR := build/ubuntu-gcc
VCPKG_DIR := $(VCPKG_DIR_LINUX)
EXE := $(BUILD_DIR)/simpleengine
endif

# AutoDoc
# -------------------------------------------------------------------------
.PHONY: help
help: ## Show this help
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.DEFAULT_GOAL := help

.PHONY: configure
configure: ## Configure CMake project
	cmake -S . -B $(BUILD_DIR) -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_DIR)/scripts/buildsystems/vcpkg.cmake

.PHONY: build
build: ## Build project
	cmake --build $(BUILD_DIR)

.PHONY: run
run: ##  Run the project

	./$(EXE)

.PHONY: clean
clean: ## Remove build directory
	rm -rf $(BUILD_DIR)
