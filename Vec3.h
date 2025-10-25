#pragma once
#include <cmath>

// =============================================================================
// Vec3: 3D Vector for 3D graphics
// =============================================================================
// WHAT'S DIFFERENT FROM 2D?
// - Third dimension (z) for depth
// - Cross product returns a vector (not scalar like 2D)
// - Used for: positions, directions, normals, colors (RGB)
//
// COORDINATE SYSTEMS (RIGHT-HANDED, OpenGL/Vulkan standard):
// - X axis: right
// - Y axis: up
// - Z axis: toward you (out of screen)
// - Left-handed (DirectX): Z points into screen
//
// MEMORY: 12 bytes (3 floats × 4 bytes)
// GPU alignment: often padded to 16 bytes (Vec4) for SIMD efficiency
// =============================================================================

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Construct from single value (useful for uniform scaling)
    explicit Vec3(float v) : x(v), y(v), z(v) {}

    // ==========================================================================
    // VECTOR ARITHMETIC (same as 2D but with Z)
    // ==========================================================================
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3& operator+=(const Vec3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3& operator-=(const Vec3& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }

    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    Vec3 operator/(float scalar) const {
        return Vec3(x / scalar, y / scalar, z / scalar);
    }

    Vec3 operator-() const {
        return Vec3(-x, -y, -z);
    }

    // Component-wise multiplication (useful for color blending, scale)
    Vec3 operator*(const Vec3& other) const {
        return Vec3(x * other.x, y * other.y, z * other.z);
    }

    // ==========================================================================
    // DOT PRODUCT (same formula, now in 3D)
    // a · b = ax*bx + ay*by + az*bz = |a||b|cos(θ)
    //
    // CRITICAL FOR 3D:
    // - Lighting: dot(normal, lightDir) gives brightness
    // - Backface culling: dot(normal, viewDir) tells if facing camera
    // - Projection: dot(v, axis) projects v onto axis
    // ==========================================================================
    float dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    // ==========================================================================
    // CROSS PRODUCT (THE BIG DIFFERENCE FROM 2D!)
    // a × b = vector perpendicular to both a and b
    //
    // Formula:
    // a × b = (ay*bz - az*by, az*bx - ax*bz, ax*by - ay*bx)
    //
    // RIGHT-HAND RULE:
    // - Point fingers along a
    // - Curl toward b
    // - Thumb points in direction of a × b
    //
    // PROPERTIES:
    // - a × b = -(b × a)  (anti-commutative)
    // - a × a = 0 (parallel vectors)
    // - |a × b| = |a||b|sin(θ)  (area of parallelogram)
    //
    // USES IN GRAPHICS:
    // - Computing surface normals: normal = edge1 × edge2
    // - Building coordinate frames: right = forward × up
    // - Determining triangle winding order
    // ==========================================================================
    Vec3 cross(const Vec3& other) const {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    // ==========================================================================
    // LENGTH (MAGNITUDE)
    // |v| = √(x² + y² + z²)  (Pythagorean theorem in 3D)
    // ==========================================================================
    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    // Squared length (faster - avoids sqrt)
    // Useful for comparisons: if (v.lengthSquared() > threshold*threshold)
    float lengthSquared() const {
        return x * x + y * y + z * z;
    }

    // ==========================================================================
    // NORMALIZATION
    // Convert to unit vector (length = 1) while preserving direction
    //
    // CRITICAL IN 3D:
    // - Surface normals MUST be normalized for lighting
    // - Direction vectors for consistency
    // - Camera vectors (forward, up, right)
    // ==========================================================================
    Vec3 normalized() const {
        float len = length();
        if (len == 0.0f) return Vec3(0, 0, 0);
        return *this / len;
    }

    void normalize() {
        float len = length();
        if (len != 0.0f) {
            x /= len;
            y /= len;
            z /= len;
        }
    }

    // ==========================================================================
    // DISTANCE
    // Distance between two points in 3D space
    // ==========================================================================
    static float distance(const Vec3& a, const Vec3& b) {
        return (b - a).length();
    }

    // ==========================================================================
    // LINEAR INTERPOLATION (LERP)
    // Blend between two vectors: lerp(a, b, 0.0) = a, lerp(a, b, 1.0) = b
    //
    // USES:
    // - Animation: smoothly move from start to end position
    // - Color gradients
    // - Smooth camera movement
    // ==========================================================================
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a + (b - a) * t;
    }

    // ==========================================================================
    // REFLECTION
    // Reflect vector v across surface with normal n
    // Formula: r = v - 2(v·n)n
    //
    // USED FOR:
    // - Mirror reflections
    // - Bouncing physics
    // - Specular lighting
    // ==========================================================================
    static Vec3 reflect(const Vec3& v, const Vec3& n) {
        return v - n * (2.0f * v.dot(n));
    }

    // ==========================================================================
    // COMMON CONSTANT VECTORS
    // ==========================================================================
    static const Vec3 ZERO;
    static const Vec3 ONE;
    static const Vec3 UNIT_X;
    static const Vec3 UNIT_Y;
    static const Vec3 UNIT_Z;
    static const Vec3 UP;      // World up (Y+)
    static const Vec3 RIGHT;   // World right (X+)
    static const Vec3 FORWARD; // World forward (Z-)
};

// Static constants
inline const Vec3 Vec3::ZERO     = Vec3(0, 0, 0);
inline const Vec3 Vec3::ONE      = Vec3(1, 1, 1);
inline const Vec3 Vec3::UNIT_X   = Vec3(1, 0, 0);
inline const Vec3 Vec3::UNIT_Y   = Vec3(0, 1, 0);
inline const Vec3 Vec3::UNIT_Z   = Vec3(0, 0, 1);
inline const Vec3 Vec3::UP       = Vec3(0, 1, 0);
inline const Vec3 Vec3::RIGHT    = Vec3(1, 0, 0);
inline const Vec3 Vec3::FORWARD  = Vec3(0, 0, -1);  // OpenGL convention: -Z is forward

// Allow scalar * vector
inline Vec3 operator*(float scalar, const Vec3& vec) {
    return vec * scalar;
}
