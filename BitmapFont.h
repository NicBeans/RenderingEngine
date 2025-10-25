#pragma once
#include "Color.h"
#include <cstdint>
#include <string>

// =============================================================================
// BitmapFont: Simple 5x7 pixel font for digits 0-9
// =============================================================================
// HOW BITMAP FONTS WORK:
// - Each character is a small grid of pixels (5 wide × 7 tall)
// - Pixels are stored as bits: 1 = draw pixel, 0 = skip
// - Very fast to render (just pixel writes, no curves/scaling)
//
// INDUSTRY USAGE:
// - Retro games (NES, Game Boy, arcade)
// - Debug overlays (FPS counters, profiling)
// - Embedded systems (limited memory)
//
// MODERN ALTERNATIVES:
// - TrueType fonts (TTF) - vector-based, scalable
// - Signed Distance Fields (SDF) - smooth at any scale
// - GPU text rendering (texture atlases)
// =============================================================================

class BitmapFont {
public:
    // Font dimensions
    // Note: Renamed from CHAR_WIDTH to avoid conflict with system macro in limits.h
    static constexpr int FONT_CHAR_WIDTH = 5;
    static constexpr int FONT_CHAR_HEIGHT = 7;

    // ==========================================================================
    // BITMAP FONT DATA
    // Each digit is 7 rows × 5 columns
    // Each row is a 5-bit pattern (bits 0-4 used)
    // 1 = draw pixel, 0 = transparent
    //
    // Example for '0':
    //   .###.   = 0b01110 = 0x0E
    //   ##.##   = 0b11011 = 0x1B
    //   ##.##   = 0b11011 = 0x1B
    //   ##.##   = 0b11011 = 0x1B
    //   ##.##   = 0b11011 = 0x1B
    //   ##.##   = 0b11011 = 0x1B
    //   .###.   = 0b01110 = 0x0E
    // ==========================================================================
    static constexpr uint8_t DIGITS[10][7] = {
        // '0'
        {
            0b01110,
            0b11011,
            0b11011,
            0b11011,
            0b11011,
            0b11011,
            0b01110
        },
        // '1'
        {
            0b00100,
            0b01100,
            0b00100,
            0b00100,
            0b00100,
            0b00100,
            0b01110
        },
        // '2'
        {
            0b01110,
            0b10001,
            0b00001,
            0b00110,
            0b01000,
            0b10000,
            0b11111
        },
        // '3'
        {
            0b11110,
            0b00001,
            0b00001,
            0b01110,
            0b00001,
            0b00001,
            0b11110
        },
        // '4'
        {
            0b00010,
            0b00110,
            0b01010,
            0b10010,
            0b11111,
            0b00010,
            0b00010
        },
        // '5'
        {
            0b11111,
            0b10000,
            0b10000,
            0b11110,
            0b00001,
            0b00001,
            0b11110
        },
        // '6'
        {
            0b01110,
            0b10000,
            0b10000,
            0b11110,
            0b10001,
            0b10001,
            0b01110
        },
        // '7'
        {
            0b11111,
            0b00001,
            0b00010,
            0b00100,
            0b01000,
            0b01000,
            0b01000
        },
        // '8'
        {
            0b01110,
            0b10001,
            0b10001,
            0b01110,
            0b10001,
            0b10001,
            0b01110
        },
        // '9'
        {
            0b01110,
            0b10001,
            0b10001,
            0b01111,
            0b00001,
            0b00001,
            0b01110
        }
    };

    // ==========================================================================
    // RENDER DIGIT
    // Draws a single digit (0-9) at position (x, y)
    //
    // Parameters:
    // - setPixel: callback function to draw a pixel
    // - digit: number to draw (0-9)
    // - x, y: top-left position
    // - color: color to draw with
    // - scale: size multiplier (1 = 5×7 pixels, 2 = 10×14 pixels, etc.)
    // ==========================================================================
    template<typename SetPixelFunc>
    static void drawDigit(SetPixelFunc setPixel, int digit, int x, int y,
                          const Color& color, int scale = 1) {
        if (digit < 0 || digit > 9) return;

        const uint8_t* bitmap = DIGITS[digit];

        // For each row
        for (int row = 0; row < FONT_CHAR_HEIGHT; row++) {
            uint8_t rowData = bitmap[row];

            // For each column (bit)
            for (int col = 0; col < FONT_CHAR_WIDTH; col++) {
                // Check if bit is set (right to left: bit 0 = rightmost)
                if (rowData & (1 << (FONT_CHAR_WIDTH - 1 - col))) {
                    // Draw scaled pixel
                    for (int sy = 0; sy < scale; sy++) {
                        for (int sx = 0; sx < scale; sx++) {
                            setPixel(x + col * scale + sx,
                                    y + row * scale + sy,
                                    color);
                        }
                    }
                }
            }
        }
    }

    // ==========================================================================
    // RENDER STRING OF DIGITS
    // Draws a string of digits (e.g., "123", "60")
    //
    // spacing: pixels between digits (default 1)
    // ==========================================================================
    template<typename SetPixelFunc>
    static void drawString(SetPixelFunc setPixel, const std::string& text,
                           int x, int y, const Color& color,
                           int scale = 1, int spacing = 1) {
        int currentX = x;

        for (char c : text) {
            if (c >= '0' && c <= '9') {
                drawDigit(setPixel, c - '0', currentX, y, color, scale);
                currentX += (FONT_CHAR_WIDTH * scale) + spacing;
            } else if (c == ' ') {
                currentX += (FONT_CHAR_WIDTH * scale) + spacing;
            }
            // Ignore non-digit, non-space characters
        }
    }

    // ==========================================================================
    // GET STRING WIDTH
    // Calculate pixel width of a string (useful for right-aligning)
    // ==========================================================================
    static int getStringWidth(const std::string& text, int scale = 1, int spacing = 1) {
        if (text.empty()) return 0;
        int charCount = 0;
        for (char c : text) {
            if ((c >= '0' && c <= '9') || c == ' ') {  // Fixed: added parentheses
                charCount++;
            }
        }
        return charCount * (FONT_CHAR_WIDTH * scale) + (charCount - 1) * spacing;
    }
};
