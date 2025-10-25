# =============================================================================
# CMAKE BUILD CONFIGURATION
# =============================================================================
# CMake is the industry-standard build system for C++ projects
# It generates platform-specific build files (Makefiles, Visual Studio, Xcode)
#
# WHAT THIS FILE DOES:
# 1. Find SDL2 library on your system
# 2. Compile all our .cpp files
# 3. Link against SDL2
# 4. Generate executable
# =============================================================================

cmake_minimum_required(VERSION 3.10)
project(Renderer)

# Use C++20 features (or C++17 if you prefer)
# C++20 gives us: concepts, ranges, coroutines, modules (some compilers)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# FIND SDL2
# CMake will search standard locations (/usr/lib, /usr/local/lib, etc.)
# =============================================================================
find_package(SDL2 REQUIRED)

# =============================================================================
# FIND OpenGL and GLEW
# Modern OpenGL for GPU rendering
# GLEW loads OpenGL extension functions
# =============================================================================
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# =============================================================================
# SOURCE FILES
# Header-only files (Vec2.h, Color.h, etc.) don't need to be listed
# They're included via #include
# =============================================================================
add_executable(Renderer
    main.cpp
    # If we had .cpp implementations, we'd list them here:
    # Vec2.cpp
    # Color.cpp
    # etc.
)

# =============================================================================
# INCLUDE DIRECTORIES
# Tell compiler where to find SDL2, OpenGL, and GLEW headers
# =============================================================================
target_include_directories(Renderer PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
)

# =============================================================================
# LINK LIBRARIES
# Link our executable against SDL2, OpenGL, and GLEW
# This connects the SDL2/OpenGL/GLEW functions we call to the actual library code
# =============================================================================
target_link_libraries(Renderer
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
)

# =============================================================================
# COMPILER FLAGS (Optional but recommended)
# =============================================================================
# Enable warnings (catch bugs early!)
if(MSVC)
    target_compile_options(Renderer PRIVATE /W4)
else()
    target_compile_options(Renderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Optimization flags for Release builds
# -O3 = maximum optimization
# -march=native = use CPU-specific instructions (SIMD, etc.)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(NOT MSVC)
        target_compile_options(Renderer PRIVATE -O3 -march=native)
    endif()
endif()
