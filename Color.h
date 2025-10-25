#pragma once
#include <cstdint>
#include <algorithm>

// =============================================================================
// Color: RGBA color representation
// =============================================================================
// INDUSTRY STANDARD FORMAT: 32-bit RGBA (8 bits per channel)
// Memory layout: [R][G][B][A] = 4 bytes = 32 bits
//
// WHY 8 BITS PER CHANNEL?
// - Human eye perceives ~10 million colors (8-bit RGB = 16.7 million)
// - 8 bits fits perfectly in a byte (CPU-friendly)
// - Total: 32 bits = 1 word on most architectures (cache-aligned)
//
// ALTERNATIVES IN INDUSTRY:
// - HDR rendering: 16 or 32 bits per channel (float)
// - Mobile/embedded: 5-6-5 RGB (16 bits total)
// - Scientific: spectral rendering (wavelength-based)
// =============================================================================

struct Color {
    uint8_t r, g, b, a;  // Range: 0-255 for each channel

    // Default: opaque black
    Color() : r(0), g(0), b(0), a(255) {}

    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    // ==========================================================================
    // FLOAT CONSTRUCTOR
    // Industry APIs often use normalized floats (0.0 - 1.0)
    // This converts from GPU-style colors to our integer format
    // ==========================================================================
    Color(float r, float g, float b, float a = 1.0f)
        : r(static_cast<uint8_t>(std::clamp(r, 0.0f, 1.0f) * 255))
        , g(static_cast<uint8_t>(std::clamp(g, 0.0f, 1.0f) * 255))
        , b(static_cast<uint8_t>(std::clamp(b, 0.0f, 1.0f) * 255))
        , a(static_cast<uint8_t>(std::clamp(a, 0.0f, 1.0f) * 255))
    {}

    // ==========================================================================
    // COLOR BLENDING (ALPHA COMPOSITING)
    // Formula: result = foreground * alpha + background * (1 - alpha)
    //
    // This is how transparency works! Used everywhere in UIs, sprites, etc.
    // The math: if alpha = 1.0, fully opaque (show foreground)
    //           if alpha = 0.0, fully transparent (show background)
    // ==========================================================================
    static Color blend(const Color& fg, const Color& bg) {
        float alpha = fg.a / 255.0f;
        float invAlpha = 1.0f - alpha;

        return Color(
            static_cast<uint8_t>(fg.r * alpha + bg.r * invAlpha),
            static_cast<uint8_t>(fg.g * alpha + bg.g * invAlpha),
            static_cast<uint8_t>(fg.b * alpha + bg.b * invAlpha),
            255  // Result is opaque
        );
    }

    // ==========================================================================
    // PACK TO 32-BIT INTEGER
    // Many APIs expect colors as 0xRRGGBBAA hex values
    // This packs our 4 bytes into a single integer
    // ==========================================================================
    uint32_t toUInt32() const {
        return (a << 24) | (b << 16) | (g << 8) | r;
    }

    // Common colors (named constants for convenience)
    static const Color BLACK;
    static const Color WHITE;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color CYAN;
    static const Color MAGENTA;
};

// =============================================================================
// Define static colors
// IMPORTANT: We use uint8_t{} to explicitly tell compiler which constructor!
// Without this, "255" could be uint8_t OR float (ambiguous!)
// =============================================================================
inline const Color Color::BLACK   = Color(uint8_t{0}, uint8_t{0}, uint8_t{0});
inline const Color Color::WHITE   = Color(uint8_t{255}, uint8_t{255}, uint8_t{255});
inline const Color Color::RED     = Color(uint8_t{255}, uint8_t{0}, uint8_t{0});
inline const Color Color::GREEN   = Color(uint8_t{0}, uint8_t{255}, uint8_t{0});
inline const Color Color::BLUE    = Color(uint8_t{0}, uint8_t{0}, uint8_t{255});
inline const Color Color::YELLOW  = Color(uint8_t{255}, uint8_t{255}, uint8_t{0});
inline const Color Color::CYAN    = Color(uint8_t{0}, uint8_t{255}, uint8_t{255});
inline const Color Color::MAGENTA = Color(uint8_t{255}, uint8_t{0}, uint8_t{255});
