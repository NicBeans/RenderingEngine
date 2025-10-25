#pragma once
#include "Color.h"
#include <vector>
#include <cstring>
#include <limits>

// =============================================================================
// Framebuffer: The 2D pixel buffer we render to
// =============================================================================
// WHAT IS A FRAMEBUFFER?
// It's literally an array of pixels. In memory, it's just:
// [pixel0][pixel1][pixel2]...[pixelN]
//
// For a 800x600 display:
// - Total pixels = 800 * 600 = 480,000 pixels
// - Each pixel = 4 bytes (RGBA)
// - Total memory = 1,920,000 bytes ≈ 1.8 MB
//
// MEMORY LAYOUT (Row-major order, standard in graphics):
// Row 0: [0,0][1,0][2,0]...[width-1,0]
// Row 1: [0,1][1,1][2,1]...[width-1,1]
// ...
//
// Formula to get pixel at (x, y):
// index = y * width + x
//
// WHY ROW-MAJOR?
// - Cache-friendly: accessing pixels left-to-right is sequential in memory
// - Matches how display hardware scans (left-to-right, top-to-bottom)
// =============================================================================

class Framebuffer {
private:
    int width;
    int height;
    std::vector<Color> pixels;  // The actual pixel data

    // ==========================================================================
    // Z-BUFFER (DEPTH BUFFER) - ESSENTIAL FOR 3D!
    // ==========================================================================
    // PROBLEM: In 3D, multiple objects can project to same screen pixel.
    // Which one should we draw?
    //
    // SOLUTION: Z-buffer - store depth at each pixel
    // - When drawing pixel: if new_depth < stored_depth, draw it and update
    // - Otherwise: skip (there's something closer in front)
    //
    // MEMORY: For 800×600: 480,000 floats × 4 bytes = 1.8 MB (same as color!)
    // Total framebuffer: 3.6 MB
    //
    // VALUES: Typically 0.0 (near) to 1.0 (far) after projection
    // Initialized to infinity (very far) so first pixel always wins
    // ==========================================================================
    std::vector<float> depthBuffer;

public:
    // ==========================================================================
    // CONSTRUCTOR
    // Allocates memory for width * height pixels
    //
    // WHY std::vector INSTEAD OF RAW ARRAY?
    // - RAII: automatic memory management (no manual delete)
    // - Exception-safe
    // - Can resize if needed
    // - Still contiguous in memory (cache-friendly)
    //
    // IN PRODUCTION: might use custom allocator for specific alignment
    // ==========================================================================
    Framebuffer(int width, int height)
        : width(width), height(height)
        , pixels(width * height, Color::BLACK)
        , depthBuffer(width * height, std::numeric_limits<float>::infinity())
    {
        // Color buffer: pre-filled with black
        // Depth buffer: pre-filled with infinity (very far away)
        // This ensures first pixel always passes depth test
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // ==========================================================================
    // PIXEL ACCESS
    // Get/Set individual pixels
    //
    // BOUNDS CHECKING: We check if (x, y) is valid
    // WHY? Buffer overruns are the #1 cause of crashes in graphics code
    //
    // OPTIMIZATION NOTE: In hot loops (inner rasterizer), we might skip checks
    // for performance. This is a trade-off between safety and speed.
    // ==========================================================================
    void setPixel(int x, int y, const Color& color) {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return;  // Silently ignore out-of-bounds (could also throw)
        }
        pixels[y * width + x] = color;
    }

    Color getPixel(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return Color::BLACK;
        }
        return pixels[y * width + x];
    }

    // ==========================================================================
    // CLEAR FRAMEBUFFER
    // Fill entire buffer with a single color
    //
    // OPTIMIZATION: memset (if color is black) or std::fill
    // Modern CPUs can clear memory VERY fast (tens of GB/s)
    // ==========================================================================
    void clear(const Color& color = Color::BLACK) {
        // std::fill is optimized by compilers, often uses SIMD
        std::fill(pixels.begin(), pixels.end(), color);
    }

    // ==========================================================================
    // CLEAR DEPTH BUFFER
    // Reset all depths to infinity (very far)
    // MUST be called at start of each 3D frame!
    // ==========================================================================
    void clearDepth() {
        std::fill(depthBuffer.begin(), depthBuffer.end(),
                  std::numeric_limits<float>::infinity());
    }

    // Clear both color and depth (typical for 3D rendering)
    void clearAll(const Color& color = Color::BLACK) {
        clear(color);
        clearDepth();
    }

    // ==========================================================================
    // DEPTH TESTING
    // Returns true if depth test passes (new pixel is closer)
    // ==========================================================================
    bool depthTest(int x, int y, float depth) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return false;  // Out of bounds
        }
        return depth < depthBuffer[y * width + x];
    }

    // Set pixel WITH depth test
    // Only draws if depth test passes
    // This is the core of 3D rendering!
    bool setPixelDepth(int x, int y, float depth, const Color& color) {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return false;
        }

        int index = y * width + x;

        // Depth test: is new pixel closer?
        if (depth < depthBuffer[index]) {
            pixels[index] = color;
            depthBuffer[index] = depth;
            return true;  // Drew pixel
        }

        return false;  // Failed depth test (something in front)
    }

    // Get depth at pixel
    float getDepth(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return std::numeric_limits<float>::infinity();
        }
        return depthBuffer[y * width + x];
    }

    // ==========================================================================
    // RAW DATA ACCESS
    // Returns pointer to the raw pixel array
    //
    // WHY NEEDED?
    // - To upload to GPU textures (OpenGL, Vulkan)
    // - To pass to display libraries (SDL)
    // - For SIMD operations (process 4-8 pixels at once)
    //
    // DANGER: Direct memory access = no bounds checking!
    // ==========================================================================
    const Color* getData() const { return pixels.data(); }
    Color* getData() { return pixels.data(); }

    // ==========================================================================
    // GET AS UINT32 ARRAY
    // Some APIs (like SDL) want pixels as 32-bit integers
    // We reinterpret our Color array as uint32_t array
    //
    // This is a REINTERPRET CAST - we're not converting data,
    // just telling the compiler "treat this memory as a different type"
    // ==========================================================================
    const uint32_t* getDataAsUInt32() const {
        return reinterpret_cast<const uint32_t*>(pixels.data());
    }
};
