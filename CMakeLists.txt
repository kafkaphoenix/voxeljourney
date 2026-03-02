cmake_minimum_required(VERSION 3.21)

project(simpleengine
    DESCRIPTION "A simple OpenGL-based game engine written in C++"
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(VERBOSE "*")
message(VERBOSE "* ${PROJECT_NAME} v${PROJECT_VERSION})")
message(VERBOSE "*")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

find_package(glad CONFIG REQUIRED)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "WIN32_LEAN_AND_MEAN" "NOMINMAX" "VC_EXTRA_LEAN") # Prevent windows.h from defining min and max macros, which interfere with std::min and std::max
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4005) # Disable warning about macro redefinition, since glad 1 defines APIENTRY, which windows.h also defines, and we have to include windows.h for memory usage tracking on Windows. Glad 2 doesn't have this issue but it's not in vcpkg yet.
endif()

find_package(glfw3 CONFIG REQUIRED)
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_NONE") # Prevent glfw from including OpenGL headers, since glad will handle that
find_package(glm CONFIG REQUIRED)
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLM_ENABLE_EXPERIMENTAL") # Enable experimental module glm::gtx https://glm.g-truc.net/0.9.0/api/a00139.html#_details
find_package(Stb REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE "STBI_FAILURE_USERMSG")
find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")
target_include_directories(${PROJECT_NAME} PRIVATE ${SIMPLEINI_INCLUDE_DIRS})
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(${PROJECT_NAME} PRIVATE ${TINYGLTF_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    glm::glm-header-only
    PRIVATE
    glad::glad
    glfw
)

# Include the src directory for header files, so we avoid having to use relative paths in the code
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Copy config.ini and assets next to the executable after building
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/config.ini
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)
