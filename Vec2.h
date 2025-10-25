#pragma once
#include <cmath>

// =============================================================================
// Vec2: 2D Vector for positions, directions, texture coordinates
// =============================================================================
// WHY THIS EXISTS:
// Rendering is 90% math. Vectors represent positions (x, y), velocities,
// normals, etc. We create our own instead of using a library so you understand
// the fundamentals. In production, you'd use GLM or Eigen.
//
// MEMORY LAYOUT:
// struct { float x, y; } = 8 bytes, tightly packed
// This is cache-friendly and GPU-compatible (matches GPU shader vec2)
// =============================================================================

struct Vec2 {
    float x, y;

    // Default constructor - initializes to (0, 0)
    // WHY: Prevents garbage values. Always initialize your memory!
    Vec2() : x(0), y(0) {}

    // Parameterized constructor
    Vec2(float x, float y) : x(x), y(y) {}

    // ==========================================================================
    // VECTOR ADDITION
    // (x1, y1) + (x2, y2) = (x1+x2, y1+y2)
    // Used for: Moving positions, combining velocities
    // ==========================================================================
    Vec2 operator+(const Vec2& other) const {
        return Vec2(x + other.x, y + other.y);
    }

    Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;  // Return reference for chaining: v += a += b
    }

    // ==========================================================================
    // VECTOR SUBTRACTION
    // Used to get direction between two points: direction = target - current
    // ==========================================================================
    Vec2 operator-(const Vec2& other) const {
        return Vec2(x - other.x, y - other.y);
    }

    // ==========================================================================
    // SCALAR MULTIPLICATION
    // Scales a vector: (x, y) * 2 = (2x, 2y)
    // Used for: Scaling, speed adjustment, interpolation
    // ==========================================================================
    Vec2 operator*(float scalar) const {
        return Vec2(x * scalar, y * scalar);
    }

    Vec2 operator/(float scalar) const {
        return Vec2(x / scalar, y / scalar);
    }

    // ==========================================================================
    // DOT PRODUCT
    // a · b = ax*bx + ay*by = |a||b|cos(θ)
    //
    // CRITICAL OPERATION in rendering:
    // - If result > 0: vectors point in similar direction
    // - If result = 0: vectors are perpendicular (90°)
    // - If result < 0: vectors point in opposite directions
    //
    // Used for: Lighting calculations, backface culling, projections
    // ==========================================================================
    float dot(const Vec2& other) const {
        return x * other.x + y * other.y;
    }

    // ==========================================================================
    // LENGTH (MAGNITUDE)
    // |v| = √(x² + y²)  (Pythagorean theorem)
    // Used for: Distance calculations, normalization
    // ==========================================================================
    float length() const {
        return std::sqrt(x * x + y * y);
    }

    // ==========================================================================
    // NORMALIZATION
    // Converts vector to unit length (length = 1) while preserving direction
    // normalized = v / |v|
    //
    // WHY: Many calculations need direction without magnitude
    // (lighting, surface normals, etc.)
    // ==========================================================================
    Vec2 normalized() const {
        float len = length();
        if (len == 0) return Vec2(0, 0);  // Prevent division by zero
        return Vec2(x / len, y / len);
    }

    // ==========================================================================
    // CROSS PRODUCT (2D VERSION)
    // In 2D, cross product returns a scalar (the Z component of 3D cross product)
    // a × b = ax*by - ay*bx
    //
    // GEOMETRIC MEANING:
    // - Positive: b is counterclockwise from a
    // - Negative: b is clockwise from a
    // - Zero: vectors are parallel
    //
    // Used for: Determining point-line relationship, triangle winding order
    // ==========================================================================
    float cross(const Vec2& other) const {
        return x * other.y - y * other.x;
    }
};

// Allow scalar * vector (in addition to vector * scalar)
// This makes "2 * vec" work in addition to "vec * 2"
inline Vec2 operator*(float scalar, const Vec2& vec) {
    return vec * scalar;
}
