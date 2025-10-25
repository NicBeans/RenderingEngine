#pragma once
#include <SDL2/SDL.h>
#include <GL/glew.h>  // Must be before gl.h
#include <GL/gl.h>
#include <stdexcept>
#include <string>
#include <iostream>

// =============================================================================
// WindowGL: OpenGL-enabled window using SDL
// =============================================================================
// DIFFERENCE FROM Window.h:
// - Creates OpenGL context instead of SDL renderer
// - No texture/framebuffer upload (GPU renders directly to screen!)
// - Much simpler display() function
//
// OPENGL CONTEXT:
// - Manages GPU state (shaders, buffers, textures, etc.)
// - All OpenGL calls affect the current context
// - SDL creates and manages the context for us
// =============================================================================

class WindowGL {
private:
    SDL_Window* window;
    SDL_GLContext glContext;  // OpenGL context (GPU state)

    int width;
    int height;

public:
    // ==========================================================================
    // CONSTRUCTOR
    // Creates window and OpenGL context
    // ==========================================================================
    WindowGL(const std::string& title, int width, int height)
        : window(nullptr), glContext(nullptr),
          width(width), height(height)
    {
        // Initialize SDL video subsystem
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw std::runtime_error("SDL_Init failed: " +
                                     std::string(SDL_GetError()));
        }

        // ======================================================================
        // OPENGL ATTRIBUTES
        // Must be set BEFORE creating window!
        // ======================================================================

        // Request OpenGL 3.3 Core Profile
        // - 3.3: Modern OpenGL (shaders required, no legacy fixed-function)
        // - Core: No deprecated features (clean, modern API)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);

        // Framebuffer configuration
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);     // 8 bits per channel
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);  // 24-bit depth buffer
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // Double buffering

        // ======================================================================
        // CREATE WINDOW
        // SDL_WINDOW_OPENGL flag tells SDL to create OpenGL-compatible window
        // ======================================================================
        window = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
        );

        if (!window) {
            SDL_Quit();
            throw std::runtime_error("SDL_CreateWindow failed: " +
                                     std::string(SDL_GetError()));
        }

        // ======================================================================
        // CREATE OPENGL CONTEXT
        // This allocates GPU resources and makes OpenGL calls valid
        // ======================================================================
        glContext = SDL_GL_CreateContext(window);

        if (!glContext) {
            SDL_DestroyWindow(window);
            SDL_Quit();
            throw std::runtime_error("SDL_GL_CreateContext failed: " +
                                     std::string(SDL_GetError()));
        }

        // ======================================================================
        // INITIALIZE GLEW
        // Loads modern OpenGL function pointers
        // Must be done after creating context!
        // ======================================================================
        glewExperimental = GL_TRUE;  // Needed for core profile
        GLenum glewError = glewInit();
        if (glewError != GLEW_OK) {
            SDL_GL_DeleteContext(glContext);
            SDL_DestroyWindow(window);
            SDL_Quit();
            throw std::runtime_error("GLEW init failed: " +
                                     std::string((const char*)glewGetErrorString(glewError)));
        }

        // ======================================================================
        // ENABLE VSYNC
        // 1 = sync to monitor refresh rate (prevents tearing)
        // 0 = no vsync (unlimited FPS, may tear)
        // -1 = adaptive vsync (fallback to 0 if can't maintain 60fps)
        // ======================================================================
        SDL_GL_SetSwapInterval(1);

        // ======================================================================
        // OPENGL INITIALIZATION
        // Set up default GPU state
        // ======================================================================

        // Enable depth testing (Z-buffer)
        // This is HARDWARE depth testing - much faster than our software version!
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);  // Closer objects win

        // Enable backface culling
        // GPU automatically discards triangles facing away from camera
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);  // Counter-clockwise winding = front face

        // Set clear color (background)
        glClearColor(60.0f/255.0f, 70.0f/255.0f, 90.0f/255.0f, 1.0f);

        // Set viewport (rendering region)
        glViewport(0, 0, width, height);

        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;
    }

    // ==========================================================================
    // DESTRUCTOR
    // Clean up OpenGL context and window
    // ==========================================================================
    ~WindowGL() {
        if (glContext) SDL_GL_DeleteContext(glContext);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    // Delete copy constructor/assignment
    WindowGL(const WindowGL&) = delete;
    WindowGL& operator=(const WindowGL&) = delete;

    // ==========================================================================
    // CLEAR AND SWAP
    // ==========================================================================

    // Clear both color and depth buffers
    // MUCH faster than software clear - GPU can clear at memory bandwidth speed
    void clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // Swap front and back buffers (double buffering)
    // This is when pixels actually appear on screen!
    void swapBuffers() {
        SDL_GL_SwapWindow(window);
    }

    // ==========================================================================
    // POLL EVENTS
    // Same as software renderer - handle window close, input, etc.
    // ==========================================================================
    bool pollEvents() {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    return false;

                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        return false;
                    }
                    break;
            }
        }

        return true;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};
