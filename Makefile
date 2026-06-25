ifeq ($(OS),Windows_NT)
EXE           := build/windows-msvc/Debug/simpleengine.exe
RENDERDOC_CMD := "C:/Program Files/RenderDoc/qrenderdoc.exe"
CONFIGURE_PRESET := windows-msvc
TIDY_PRESET      := windows-tidy
else
EXE           := build/ubuntu-gcc/simpleengine
RENDERDOC_CMD := renderdoc
CONFIGURE_PRESET := ubuntu-gcc
TIDY_PRESET      := ubuntu-tidy
endif

# -------------------------------------------------------------------------
.DEFAULT_GOAL := help

.PHONY: help
help: ## Show this help
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

.PHONY: configure
configure: ## Configure CMake project
	cmake --preset $(CONFIGURE_PRESET)

.PHONY: build
build: ## Build project
	cmake --build --preset $(CONFIGURE_PRESET)-debug

.PHONY: run
run: ## Run the project
	$(EXE)

.PHONY: clean
clean: ## Remove build directory
	rm -rf build

.PHONY: renderdoc
renderdoc: ## Run RenderDoc
	$(RENDERDOC_CMD) capture $(EXE) --wait-for-exit

.PHONY: tidy
tidy: ## Run clang-tidy static analysis
	cmake --preset $(TIDY_PRESET)
	cmake --build --preset $(TIDY_PRESET)

.PHONY: format
format: ## Run clang-format on all source files
	find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i