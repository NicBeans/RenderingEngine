#pragma once
#include "Vec3.h"
#include "Vec4.h"
#include <cmath>
#include <cstring>

// =============================================================================
// Mat4: 4×4 Matrix for 3D Transformations
// =============================================================================
// THE FOUNDATION OF 3D GRAPHICS!
//
// Every 3D transformation is a matrix:
// - Translation (move)
// - Rotation (turn)
// - Scale (resize)
// - Projection (3D → 2D)
// - View (camera positioning)
//
// WHY 4×4 FOR 3D?
// - Homogeneous coordinates (Vec4) allow translation to be matrix multiplication
// - Combining transforms: matrix_combined = projection * view * model
// - Single matrix multiply applies ALL transformations!
//
// MEMORY LAYOUT (Column-Major, OpenGL/Vulkan standard):
// m[0]  m[4]  m[8]  m[12]    Column 0  Column 1  Column 2  Column 3
// m[1]  m[5]  m[9]  m[13]
// m[2]  m[6]  m[10] m[14]
// m[3]  m[7]  m[11] m[15]
//
// For a transform matrix:
// [Xx Yx Zx Tx]  ← X axis direction (Xx,Xy,Xz), Translation (Tx,Ty,Tz)
// [Xy Yy Zy Ty]  ← Y axis direction
// [Xz Yz Zz Tz]  ← Z axis direction
// [0  0  0  1 ]  ← Homogeneous row
//
// WHY COLUMN-MAJOR?
// - OpenGL standard
// - Matrix * Vector order: result = M * v
// - DirectX uses row-major (result = v * M)
// =============================================================================

struct Mat4 {
    // Column-major storage (OpenGL convention)
    float m[16];

    // ==========================================================================
    // CONSTRUCTORS
    // ==========================================================================
    Mat4() {
        // Initialize to identity by default
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    // Construct from 16 floats (column-major order!)
    Mat4(float m0,  float m1,  float m2,  float m3,
         float m4,  float m5,  float m6,  float m7,
         float m8,  float m9,  float m10, float m11,
         float m12, float m13, float m14, float m15) {
        m[0]  = m0;  m[1]  = m1;  m[2]  = m2;  m[3]  = m3;
        m[4]  = m4;  m[5]  = m5;  m[6]  = m6;  m[7]  = m7;
        m[8]  = m8;  m[9]  = m9;  m[10] = m10; m[11] = m11;
        m[12] = m12; m[13] = m13; m[14] = m14; m[15] = m15;
    }

    // ==========================================================================
    // MATRIX MULTIPLICATION
    // THE MOST IMPORTANT OPERATION!
    //
    // Combines transformations: M = A * B means "do B first, then A"
    // Example: projection * view * model
    // 1. Model transform (object space → world space)
    // 2. View transform (world space → camera space)
    // 3. Projection (camera space → clip space)
    //
    // PERFORMANCE: 64 multiplies + 48 adds per matrix multiply
    // GPUs have dedicated hardware for this!
    // ==========================================================================
    Mat4 operator*(const Mat4& other) const {
        Mat4 result;

        // For each column of result (j)
        for (int j = 0; j < 4; j++) {
            // For each row of result (i)
            for (int i = 0; i < 4; i++) {
                float sum = 0.0f;
                // Dot product of row i from *this with column j from other
                for (int k = 0; k < 4; k++) {
                    sum += m[k * 4 + i] * other.m[j * 4 + k];
                }
                result.m[j * 4 + i] = sum;
            }
        }

        return result;
    }

    // ==========================================================================
    // MATRIX-VECTOR MULTIPLICATION
    // Transform a point/vector: result = M * v
    //
    // This is how we transform vertices!
    // Each vertex is multiplied by model, view, projection matrices
    // ==========================================================================
    Vec4 operator*(const Vec4& v) const {
        return Vec4(
            m[0]*v.x + m[4]*v.y + m[8]*v.z  + m[12]*v.w,  // Row 0 · v
            m[1]*v.x + m[5]*v.y + m[9]*v.z  + m[13]*v.w,  // Row 1 · v
            m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w,  // Row 2 · v
            m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w   // Row 3 · v
        );
    }

    // Transform Vec3 as point (w=1, affected by translation)
    Vec3 transformPoint(const Vec3& v) const {
        Vec4 result = (*this) * Vec4(v, 1.0f);
        return result.toVec3();  // Perspective divide
    }

    // Transform Vec3 as direction (w=0, NOT affected by translation)
    Vec3 transformDirection(const Vec3& v) const {
        Vec4 result = (*this) * Vec4(v, 0.0f);
        return result.xyz();  // No perspective divide for directions
    }

    // ==========================================================================
    // IDENTITY MATRIX
    // [1 0 0 0]
    // [0 1 0 0]  ← No transformation (M * v = v)
    // [0 0 1 0]
    // [0 0 0 1]
    // ==========================================================================
    static Mat4 identity() {
        return Mat4();  // Constructor already makes identity
    }

    // ==========================================================================
    // TRANSLATION MATRIX
    // [1 0 0 tx]
    // [0 1 0 ty]  ← Moves object by (tx, ty, tz)
    // [0 0 1 tz]
    // [0 0 0  1]
    //
    // Effect: (x, y, z) → (x+tx, y+ty, z+tz)
    // ==========================================================================
    static Mat4 translate(const Vec3& t) {
        Mat4 result;
        result.m[12] = t.x;
        result.m[13] = t.y;
        result.m[14] = t.z;
        return result;
    }

    static Mat4 translate(float x, float y, float z) {
        return translate(Vec3(x, y, z));
    }

    // ==========================================================================
    // SCALE MATRIX
    // [sx 0  0  0]
    // [0  sy 0  0]  ← Scales object by (sx, sy, sz)
    // [0  0  sz 0]
    // [0  0  0  1]
    //
    // Effect: (x, y, z) → (x*sx, y*sy, z*sz)
    // ==========================================================================
    static Mat4 scale(const Vec3& s) {
        Mat4 result;
        result.m[0]  = s.x;
        result.m[5]  = s.y;
        result.m[10] = s.z;
        return result;
    }

    static Mat4 scale(float x, float y, float z) {
        return scale(Vec3(x, y, z));
    }

    static Mat4 scale(float s) {
        return scale(s, s, s);  // Uniform scale
    }

    // ==========================================================================
    // ROTATION MATRICES
    // ==========================================================================

    // Rotate around X axis (pitch - look up/down)
    // [1    0       0    0]
    // [0  cos(θ) -sin(θ) 0]
    // [0  sin(θ)  cos(θ) 0]
    // [0    0       0    1]
    static Mat4 rotateX(float angleRadians) {
        Mat4 result;
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        result.m[5]  =  c;
        result.m[6]  =  s;
        result.m[9]  = -s;
        result.m[10] =  c;
        return result;
    }

    // Rotate around Y axis (yaw - look left/right)
    // [ cos(θ) 0 sin(θ) 0]
    // [   0    1   0    0]
    // [-sin(θ) 0 cos(θ) 0]
    // [   0    0   0    1]
    static Mat4 rotateY(float angleRadians) {
        Mat4 result;
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        result.m[0]  =  c;
        result.m[2]  = -s;
        result.m[8]  =  s;
        result.m[10] =  c;
        return result;
    }

    // Rotate around Z axis (roll - tilt head)
    // [cos(θ) -sin(θ) 0 0]
    // [sin(θ)  cos(θ) 0 0]
    // [  0       0    1 0]
    // [  0       0    0 1]
    static Mat4 rotateZ(float angleRadians) {
        Mat4 result;
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        result.m[0] =  c;
        result.m[1] =  s;
        result.m[4] = -s;
        result.m[5] =  c;
        return result;
    }

    // ==========================================================================
    // LOOK-AT MATRIX (VIEW MATRIX)
    // ==========================================================================
    // Creates a camera transformation
    // - eye: camera position
    // - target: point camera is looking at
    // - up: which direction is "up" (usually (0,1,0))
    //
    // HOW IT WORKS:
    // 1. Calculate camera's local axes (forward, right, up)
    // 2. Build rotation that aligns these with world axes
    // 3. Apply translation to move camera to origin
    //
    // This is THE view matrix for cameras!
    // ==========================================================================
    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        // Calculate camera's forward vector (points from eye to target)
        Vec3 forward = (target - eye).normalized();

        // Calculate camera's right vector (perpendicular to forward and up)
        Vec3 right = forward.cross(up).normalized();

        // Recalculate up vector (perpendicular to forward and right)
        // This ensures orthogonality even if input 'up' wasn't perpendicular
        Vec3 cameraUp = right.cross(forward);

        // Build rotation + translation matrix
        // NOTE: forward is negated because in OpenGL, camera looks down -Z
        Mat4 result;
        result.m[0] = right.x;
        result.m[4] = right.y;
        result.m[8] = right.z;

        result.m[1] = cameraUp.x;
        result.m[5] = cameraUp.y;
        result.m[9] = cameraUp.z;

        result.m[2]  = -forward.x;
        result.m[6]  = -forward.y;
        result.m[10] = -forward.z;

        // Translation part (move world opposite to camera position)
        result.m[12] = -right.dot(eye);
        result.m[13] = -cameraUp.dot(eye);
        result.m[14] = forward.dot(eye);

        return result;
    }

    // ==========================================================================
    // PERSPECTIVE PROJECTION MATRIX
    // ==========================================================================
    // THE MAGIC THAT MAKES 3D LOOK 3D!
    //
    // Parameters:
    // - fovY: Field of view in Y axis (radians). Typical: 45° = 0.785 rad
    // - aspect: width/height ratio. For 800×600: 800/600 = 1.333
    // - near: Near clipping plane (objects closer are clipped). Typical: 0.1
    // - far: Far clipping plane (objects farther are clipped). Typical: 100
    //
    // HOW PERSPECTIVE WORKS:
    // 1. Matrix scales x,y based on z (depth)
    // 2. Stores depth in w component
    // 3. Later: x/w, y/w makes distant objects smaller!
    //
    // FIELD OF VIEW (FOV):
    // - Small FOV (30°): telephoto lens, zoom in
    // - Medium FOV (60-90°): normal human vision
    // - Large FOV (120°+): fish-eye lens, wide angle
    // ==========================================================================
    static Mat4 perspective(float fovYRadians, float aspect, float near, float far) {
        Mat4 result;
        std::memset(result.m, 0, sizeof(result.m));

        float tanHalfFov = std::tan(fovYRadians / 2.0f);

        // Scale X based on FOV and aspect ratio
        result.m[0] = 1.0f / (aspect * tanHalfFov);

        // Scale Y based on FOV
        result.m[5] = 1.0f / tanHalfFov;

        // Z mapping: map [near, far] to [-1, 1] (OpenGL clip space)
        result.m[10] = -(far + near) / (far - near);
        result.m[11] = -1.0f;  // This puts depth into w

        // Z translation component
        result.m[14] = -(2.0f * far * near) / (far - near);

        return result;
    }

    // ==========================================================================
    // ORTHOGRAPHIC PROJECTION MATRIX
    // ==========================================================================
    // Parallel projection (no perspective - distant objects same size)
    // Used for: 2D games, UI, CAD, isometric games
    //
    // Maps:
    // x: [left, right] → [-1, 1]
    // y: [bottom, top] → [-1, 1]
    // z: [near, far] → [-1, 1]
    // ==========================================================================
    static Mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
        Mat4 result;
        std::memset(result.m, 0, sizeof(result.m));

        result.m[0]  = 2.0f / (right - left);
        result.m[5]  = 2.0f / (top - bottom);
        result.m[10] = -2.0f / (far - near);
        result.m[12] = -(right + left) / (right - left);
        result.m[13] = -(top + bottom) / (top - bottom);
        result.m[14] = -(far + near) / (far - near);
        result.m[15] = 1.0f;

        return result;
    }
};
