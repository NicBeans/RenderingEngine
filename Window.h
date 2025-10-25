#pragma once
#include <SDL2/SDL.h>
#include "Framebuffer.h"
#include <stdexcept>
#include <string>

// =============================================================================
// Window: Cross-platform window management using SDL2
// =============================================================================
// WHAT IS SDL2?
// Simple DirectMedia Layer - industry-standard library for:
// - Window creation (Windows, Linux, macOS)
// - Input handling (keyboard, mouse, gamepad)
// - Audio
// - Basic 2D rendering
//
// USED BY: Valve (Steam), many indie games, emulators, multimedia apps
//
// ALTERNATIVES:
// - GLFW (more minimal, OpenGL-focused)
// - Platform-specific: Win32 API, X11, Cocoa
// - Qt, wxWidgets (full UI frameworks)
//
// WHY SDL2 FOR LEARNING?
// - Simple API
// - Cross-platform (write once, run anywhere)
// - Well-documented
// - Can display our framebuffer directly OR use it with OpenGL
// =============================================================================

class Window {
private:
    SDL_Window* window;      // OS window handle
    SDL_Renderer* renderer;  // SDL's 2D renderer (NOT our renderer!)
    SDL_Texture* texture;    // GPU texture to display our framebuffer

    int width;
    int height;

public:
    // ==========================================================================
    // CONSTRUCTOR
    // Initializes SDL and creates a window
    //
    // WHAT HAPPENS UNDER THE HOOD:
    // 1. SDL initializes video subsystem (talks to OS)
    // 2. OS allocates window resources (memory, handles)
    // 3. SDL creates renderer (talks to GPU driver)
    // 4. We create texture (GPU memory for our framebuffer)
    // ==========================================================================
    Window(const std::string& title, int width, int height)
        : window(nullptr), renderer(nullptr), texture(nullptr),
          width(width), height(height)
    {
        // Initialize SDL video subsystem
        // SDL_INIT_VIDEO = window + rendering capabilities
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw std::runtime_error("SDL_Init failed: " +
                                     std::string(SDL_GetError()));
        }

        // ======================================================================
        // CREATE WINDOW
        // SDL_WINDOW_SHOWN = make visible immediately
        // Position: SDL_WINDOWPOS_CENTERED = center on screen
        // ======================================================================
        window = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            SDL_WINDOW_SHOWN
        );

        if (!window) {
            SDL_Quit();
            throw std::runtime_error("SDL_CreateWindow failed: " +
                                     std::string(SDL_GetError()));
        }

        // ======================================================================
        // CREATE RENDERER
        // SDL_Renderer is SDL's abstraction over GPU rendering
        // -1 = use first available driver (usually hardware accelerated)
        // SDL_RENDERER_ACCELERATED = use GPU (not software)
        // SDL_RENDERER_PRESENTVSYNC = sync to monitor refresh (prevents tearing)
        // ======================================================================
        renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );

        if (!renderer) {
            SDL_DestroyWindow(window);
            SDL_Quit();
            throw std::runtime_error("SDL_CreateRenderer failed: " +
                                     std::string(SDL_GetError()));
        }

        // ======================================================================
        // CREATE TEXTURE
        // This is GPU memory where we'll upload our framebuffer
        //
        // PIXEL FORMAT BYTE ORDER EXPLANATION:
        // Our Color struct in memory: [R byte][G byte][B byte][A byte]
        //
        // SDL formats on little-endian (x86-64):
        // - SDL_PIXELFORMAT_RGBA8888 = 0xRRGGBBAA as uint32 = [AA][BB][GG][RR] in bytes
        // - SDL_PIXELFORMAT_ABGR8888 = 0xAABBGGRR as uint32 = [RR][GG][BB][AA] in bytes ✓
        //
        // Our struct matches ABGR8888 byte order!
        // ======================================================================
        texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ABGR8888,  // Matches our [R][G][B][A] byte layout
            SDL_TEXTUREACCESS_STREAMING,
            width, height
        );

        if (!texture) {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            throw std::runtime_error("SDL_CreateTexture failed: " +
                                     std::string(SDL_GetError()));
        }
    }

    // ==========================================================================
    // DESTRUCTOR
    // RAII pattern: Resource Acquisition Is Initialization
    // Constructor allocates resources, destructor frees them
    // This prevents memory leaks!
    // ==========================================================================
    ~Window() {
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    // Delete copy constructor/assignment (we own SDL resources uniquely)
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // ==========================================================================
    // DISPLAY FRAMEBUFFER
    // This is the critical function: gets our CPU pixels onto the screen!
    //
    // THE PIPELINE:
    // 1. Our framebuffer (system RAM) → SDL texture (GPU VRAM)
    // 2. Texture → GPU render target
    // 3. Render target → Display via vsync
    //
    // PERFORMANCE NOTE:
    // Copying RAM → VRAM is relatively slow (PCIe bandwidth: ~16 GB/s)
    // For 1920x1080 @ 60fps = ~500 MB/s (manageable)
    // For comparison, GPU → GPU copies can be 100x faster!
    // ==========================================================================
    void display(const Framebuffer& fb) {
        // ======================================================================
        // UPLOAD FRAMEBUFFER TO GPU
        // SDL_UpdateTexture copies pixel data from RAM to VRAM
        //
        // Parameters:
        // - texture: destination (GPU)
        // - nullptr: update entire texture (could update sub-rectangle)
        // - pixels: source data (CPU)
        // - pitch: bytes per row = width * 4 (RGBA)
        // ======================================================================
        SDL_UpdateTexture(
            texture,
            nullptr,  // Update entire texture
            fb.getData(),  // Our pixel data
            fb.getWidth() * sizeof(Color)  // Pitch (bytes per row)
        );

        // ======================================================================
        // CLEAR RENDER TARGET
        // Good practice: clear before drawing
        // ======================================================================
        SDL_RenderClear(renderer);

        // ======================================================================
        // COPY TEXTURE TO RENDER TARGET
        // This queues a GPU blit (block image transfer)
        // Texture → backbuffer
        // ======================================================================
        SDL_RenderCopy(
            renderer,
            texture,
            nullptr,  // Source rectangle (nullptr = entire texture)
            nullptr   // Destination rectangle (nullptr = entire window)
        );

        // ======================================================================
        // PRESENT (SWAP BUFFERS)
        // This is where pixels actually appear on screen!
        //
        // DOUBLE BUFFERING:
        // We have 2 buffers: front (displayed) and back (drawing to)
        // SDL_RenderPresent swaps them atomically
        //
        // WHY? Prevents tearing (showing half-old, half-new frame)
        //
        // VSYNC: If enabled, this waits for monitor refresh (60Hz = 16.67ms)
        // ======================================================================
        SDL_RenderPresent(renderer);
    }

    // ==========================================================================
    // POLL EVENTS
    // Handle window events (close, resize, input, etc.)
    // Returns false if user closed the window
    //
    // IMPORTANT: Must be called regularly or OS thinks app is frozen!
    // ==========================================================================
    bool pollEvents() {
        SDL_Event event;

        // Process all pending events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:  // User clicked X button
                    return false;

                case SDL_KEYDOWN:  // Key pressed
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        return false;  // ESC to quit
                    }
                    break;

                // Could handle: SDL_MOUSEMOTION, SDL_MOUSEBUTTONDOWN, etc.
            }
        }

        return true;  // Keep running
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};
