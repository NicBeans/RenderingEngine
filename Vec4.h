#pragma once
#include "Vec3.h"
#include <cmath>

// =============================================================================
// Vec4: Homogeneous Coordinates (THE SECRET SAUCE OF 3D GRAPHICS!)
// =============================================================================
// WHY DO WE NEED A 4TH DIMENSION?
//
// Problem: With 3D vectors, you can do rotation and scale with 3×3 matrices,
// but you CAN'T do translation (moving objects)!
//
// Example (doesn't work with 3D):
// [x']   [? ? ?] [x]   [tx]
// [y'] = [? ? ?] [y] + [ty]  ← Need addition, can't express as matrix mult!
// [z']   [? ? ?] [z]   [tz]
//
// SOLUTION: Add a 4th component (w) and use 4×4 matrices!
// Now translation is part of matrix multiplication:
//
// [x']   [1 0 0 tx] [x]
// [y']   [0 1 0 ty] [y]
// [z'] = [0 0 1 tz] [z]
// [w']   [0 0 0  1] [w]
//
// HOMOGENEOUS COORDINATES:
// - 3D point: (x, y, z, 1)  ← w=1 means it's a position
// - 3D direction: (x, y, z, 0)  ← w=0 means it's a vector (no translation!)
//
// This distinction is CRITICAL:
// - Translate a point: moves it
// - Translate a direction: no effect (directions don't have position!)
//
// PERSPECTIVE DIVISION:
// After projection, w ≠ 1. We divide by w to get screen coordinates:
// screen_x = x / w
// screen_y = y / w
// This is how perspective works (things far away appear smaller)!
//
// MEMORY: 16 bytes (4 floats × 4 bytes) - perfect for SIMD (SSE, AVX)
// =============================================================================

struct Vec4 {
    float x, y, z, w;

    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    // Construct from Vec3 + w
    // Most common: Vec4(position, 1.0f) or Vec4(direction, 0.0f)
    Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    // Explicit conversion to Vec3 (drops w component)
    Vec3 xyz() const {
        return Vec3(x, y, z);
    }

    // ==========================================================================
    // PERSPECTIVE DIVISION
    // Convert homogeneous coordinates back to 3D
    // This is THE KEY to perspective projection!
    //
    // After projection matrix:
    // - x, y, z are scaled by distance from camera
    // - w contains the depth information
    // - Dividing by w makes distant objects appear smaller!
    // ==========================================================================
    Vec3 toVec3() const {
        if (w == 0.0f) return Vec3(x, y, z);  // Direction vector
        return Vec3(x / w, y / w, z / w);     // Perspective divide
    }

    // ==========================================================================
    // VECTOR ARITHMETIC
    // ==========================================================================
    Vec4 operator+(const Vec4& other) const {
        return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    Vec4 operator-(const Vec4& other) const {
        return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    Vec4 operator*(float scalar) const {
        return Vec4(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    Vec4 operator/(float scalar) const {
        return Vec4(x / scalar, y / scalar, z / scalar, w / scalar);
    }

    // ==========================================================================
    // DOT PRODUCT
    // Used in matrix-vector multiplication (each row of matrix · vector)
    // ==========================================================================
    float dot(const Vec4& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    // ==========================================================================
    // LENGTH
    // ==========================================================================
    float length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }

    float lengthSquared() const {
        return x * x + y * y + z * z + w * w;
    }

    Vec4 normalized() const {
        float len = length();
        if (len == 0.0f) return Vec4(0, 0, 0, 0);
        return *this / len;
    }
};

// Allow scalar * vector
inline Vec4 operator*(float scalar, const Vec4& vec) {
    return vec * scalar;
}
