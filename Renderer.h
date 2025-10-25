#pragma once
#include "Framebuffer.h"
#include "Vec2.h"
#include "Color.h"
#include "BitmapFont.h"
#include <algorithm>
#include <vector>
#include <string>

// =============================================================================
// Renderer: Software rasterizer for 2D shapes
// =============================================================================
// WHAT IS RASTERIZATION?
// Converting geometric shapes (lines, triangles) into pixels. This is the
// core of ALL real-time graphics (games, UIs, etc.)
//
// HARDWARE vs SOFTWARE:
// - Software (what we're doing): CPU calculates each pixel
// - Hardware (GPU): Specialized circuits do this in parallel (1000x faster)
//
// WHY LEARN SOFTWARE RASTERIZATION?
// - Understand what the GPU actually does
// - Debugging: can step through pixel-by-pixel
// - Special effects: sometimes CPU is better for certain algorithms
// =============================================================================

class Renderer {
protected:
    // Protected allows derived classes (Renderer3D) to access framebuffer
    // This is a common pattern in class hierarchies
    Framebuffer& framebuffer;  // Reference to the pixel buffer we draw into

public:
    explicit Renderer(Framebuffer& fb) : framebuffer(fb) {}

    // ==========================================================================
    // DRAW PIXEL
    // Most basic operation - sets a single pixel to a color
    // Everything else builds on this!
    // ==========================================================================
    void drawPixel(int x, int y, const Color& color) {
        framebuffer.setPixel(x, y, color);
    }

    // ==========================================================================
    // DRAW LINE - Bresenham's Algorithm (1962)
    // ==========================================================================
    // PROBLEM: How do you draw a line from (x0,y0) to (x1,y1)?
    // Naive approach: y = mx + b, calculate y for each x
    // But this uses FLOATING POINT (slow) and has gaps for steep lines!
    //
    // BRESENHAM'S SOLUTION: Use only INTEGER arithmetic
    // Idea: at each x, decide if we go (x+1, y) or (x+1, y+1)
    // Uses an "error" term to track when to step in y
    //
    // WHY THIS ALGORITHM?
    // - Pure integer math (was crucial in 1960s, still faster today)
    // - No gaps, no antialiasing (we'll add that later if you want)
    // - Industry standard for 60+ years!
    //
    // OPTIMIZATION: Modern CPUs can do float math fast, but this is still
    // used because it's predictable and works well with fixed-point math
    // ==========================================================================
    void drawLine(int x0, int y0, int x1, int y1, const Color& color) {
        // Calculate deltas (how far we need to go)
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);

        // Determine direction: are we going left/right, up/down?
        int sx = (x0 < x1) ? 1 : -1;  // Step direction in x
        int sy = (y0 < y1) ? 1 : -1;  // Step direction in y

        // Error term: tracks when we need to step in the minor axis
        int err = dx - dy;

        // Loop until we reach the end point
        while (true) {
            drawPixel(x0, y0, color);

            // Reached destination?
            if (x0 == x1 && y0 == y1) break;

            // Calculate error for next step
            int e2 = 2 * err;

            // Should we step in x?
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }

            // Should we step in y?
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    // Overload for Vec2
    void drawLine(const Vec2& p0, const Vec2& p1, const Color& color) {
        drawLine(static_cast<int>(p0.x), static_cast<int>(p0.y),
                 static_cast<int>(p1.x), static_cast<int>(p1.y), color);
    }

    // ==========================================================================
    // DRAW TRIANGLE (FILLED) - Scanline Rasterization
    // ==========================================================================
    // TRIANGLES ARE FUNDAMENTAL IN 3D GRAPHICS
    // Why triangles?
    // 1. Always planar (3 points define a plane)
    // 2. Can approximate any curved surface
    // 3. Simple to rasterize
    // 4. GPUs are optimized for them
    //
    // ALGORITHM: Scanline rasterization
    // For each horizontal line (scanline) that intersects the triangle:
    // 1. Find where the scanline enters and exits the triangle
    // 2. Fill pixels between entry and exit
    //
    // This is how GPUs do it! (with massive parallelism)
    // ==========================================================================
    void drawTriangle(Vec2 v0, Vec2 v1, Vec2 v2, const Color& color) {
        // ======================================================================
        // STEP 1: Sort vertices by Y coordinate (top to bottom)
        // Makes the algorithm simpler - we always process top-to-bottom
        // ======================================================================
        if (v0.y > v1.y) std::swap(v0, v1);
        if (v0.y > v2.y) std::swap(v0, v2);
        if (v1.y > v2.y) std::swap(v1, v2);
        // Now: v0.y <= v1.y <= v2.y

        // ======================================================================
        // STEP 2: Rasterize top half (v0 to v1)
        // ======================================================================
        fillFlatBottomTriangle(v0, v1, v2, color);
    }

    // ==========================================================================
    // DRAW CIRCLE - Midpoint Circle Algorithm
    // ==========================================================================
    // PROBLEM: Circle equation is x² + y² = r²
    // If we solve for y = √(r² - x²), we need square roots (SLOW!)
    //
    // SOLUTION: Midpoint algorithm (similar to Bresenham)
    // Uses 8-way symmetry: if we draw 1/8 of circle, we can mirror it 8 times
    // Only uses integer arithmetic!
    // ==========================================================================
    void drawCircle(int cx, int cy, int radius, const Color& color) {
        int x = 0;
        int y = radius;
        int d = 1 - radius;  // Decision parameter

        // Helper lambda to draw all 8 symmetric points
        auto drawSymmetricPoints = [&](int x, int y) {
            drawPixel(cx + x, cy + y, color);
            drawPixel(cx - x, cy + y, color);
            drawPixel(cx + x, cy - y, color);
            drawPixel(cx - x, cy - y, color);
            drawPixel(cx + y, cy + x, color);
            drawPixel(cx - y, cy + x, color);
            drawPixel(cx + y, cy - x, color);
            drawPixel(cx - y, cy - x, color);
        };

        while (x <= y) {
            drawSymmetricPoints(x, y);

            if (d < 0) {
                d += 2 * x + 3;
            } else {
                d += 2 * (x - y) + 5;
                y--;
            }
            x++;
        }
    }

    // Overload for Vec2
    void drawCircle(const Vec2& center, int radius, const Color& color) {
        drawCircle(static_cast<int>(center.x), static_cast<int>(center.y),
                   radius, color);
    }

    // ==========================================================================
    // DRAW TEXT (using bitmap font)
    // Renders text using a simple 5x7 bitmap font
    //
    // Parameters:
    // - text: string to draw (digits 0-9 and spaces supported)
    // - x, y: top-left position
    // - color: text color
    // - scale: size multiplier (1 = small, 2 = medium, 3 = large, etc.)
    // - spacing: pixels between characters
    //
    // USAGE:
    // renderer.drawText("60", 10, 10, Color::WHITE, 2);  // Draw "60" at (10,10), 2x size
    // ==========================================================================
    void drawText(const std::string& text, int x, int y, const Color& color,
                  int scale = 1, int spacing = 1) {
        // Lambda to capture 'this' and call drawPixel
        auto setPixel = [this](int px, int py, const Color& c) {
            this->drawPixel(px, py, c);
        };

        BitmapFont::drawString(setPixel, text, x, y, color, scale, spacing);
    }

    // Get pixel width of text (useful for right-alignment)
    int getTextWidth(const std::string& text, int scale = 1, int spacing = 1) const {
        return BitmapFont::getStringWidth(text, scale, spacing);
    }

    // ==========================================================================
    // FILLED CIRCLE
    // Just like outline circle, but fill horizontal spans
    // ==========================================================================
    void drawFilledCircle(int cx, int cy, int radius, const Color& color) {
        int x = 0;
        int y = radius;
        int d = 1 - radius;

        auto drawHorizontalLine = [&](int x1, int x2, int y) {
            if (x1 > x2) std::swap(x1, x2);
            for (int x = x1; x <= x2; x++) {
                drawPixel(x, y, color);
            }
        };

        while (x <= y) {
            drawHorizontalLine(cx - x, cx + x, cy + y);
            drawHorizontalLine(cx - x, cx + x, cy - y);
            drawHorizontalLine(cx - y, cx + y, cy + x);
            drawHorizontalLine(cx - y, cx + y, cy - x);

            if (d < 0) {
                d += 2 * x + 3;
            } else {
                d += 2 * (x - y) + 5;
                y--;
            }
            x++;
        }
    }

    // ==========================================================================
    // DRAW POLYGON
    // Draws an arbitrary polygon given an array of vertices
    // Uses line drawing for the edges
    // ==========================================================================
    void drawPolygon(const std::vector<Vec2>& vertices, const Color& color) {
        if (vertices.size() < 2) return;

        // Draw edges
        for (size_t i = 0; i < vertices.size(); i++) {
            const Vec2& p0 = vertices[i];
            const Vec2& p1 = vertices[(i + 1) % vertices.size()];
            drawLine(p0, p1, color);
        }
    }

private:
    // ==========================================================================
    // HELPER: Fill flat-bottom triangle
    // This is a simplified version - full implementation would split triangles
    // into flat-top and flat-bottom pieces for complete scanline rasterization
    // ==========================================================================
    void fillFlatBottomTriangle(Vec2 v0, Vec2 v1, Vec2 v2, const Color& color) {
        // Simplified: just fill with horizontal lines
        // A production rasterizer would interpolate colors, textures, etc.

        // Get bounding box
        int minX = static_cast<int>(std::min({v0.x, v1.x, v2.x}));
        int maxX = static_cast<int>(std::max({v0.x, v1.x, v2.x}));
        int minY = static_cast<int>(std::min({v0.y, v1.y, v2.y}));
        int maxY = static_cast<int>(std::max({v0.y, v1.y, v2.y}));

        // For each pixel in bounding box, check if it's inside triangle
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                Vec2 p(static_cast<float>(x), static_cast<float>(y));

                // Use barycentric coordinates to test if point is inside
                if (isPointInTriangle(p, v0, v1, v2)) {
                    drawPixel(x, y, color);
                }
            }
        }
    }

    // ==========================================================================
    // POINT-IN-TRIANGLE TEST (Barycentric Coordinates)
    // ==========================================================================
    // PROBLEM: Given point P and triangle (v0, v1, v2), is P inside?
    //
    // SOLUTION: Barycentric coordinates
    // Express P as: P = w0*v0 + w1*v1 + w2*v2 where w0+w1+w2 = 1
    // If all weights (w0, w1, w2) are >= 0, point is inside!
    //
    // IMPLEMENTATION: Use cross products (sign tells us which side of edge)
    // This is the SAME math GPUs use for triangle rasterization!
    // ==========================================================================
    bool isPointInTriangle(const Vec2& p, const Vec2& v0, const Vec2& v1, const Vec2& v2) {
        // Calculate barycentric coordinates using cross products
        float d00 = (v1 - v0).cross(v2 - v0);
        float d01 = (v1 - v0).cross(p - v0);
        float d02 = (p - v0).cross(v2 - v0);

        // Check if same sign (same side of all edges)
        float w1 = d02 / d00;
        float w2 = d01 / d00;
        float w0 = 1.0f - w1 - w2;

        return (w0 >= 0) && (w1 >= 0) && (w2 >= 0);
    }
};
