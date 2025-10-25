#pragma once
#include "Vec3.h"
#include "Mat4.h"
#include <cmath>

// =============================================================================
// Camera: 3D Camera with View and Projection
// =============================================================================
// A camera defines HOW we look at the 3D world
//
// TWO KEY MATRICES:
// 1. VIEW MATRIX: Where is the camera? What is it looking at?
// 2. PROJECTION MATRIX: How do we convert 3D → 2D? (perspective/ortho)
//
// COMBINED: MVP = Projection * View * Model
// - Model: object space → world space (where is the object?)
// - View: world space → camera space (where is the camera?)
// - Projection: camera space → clip space → screen (perspective)
//
// This is THE fundamental pipeline of 3D graphics!
// =============================================================================

class Camera {
private:
    // Camera position and orientation
    Vec3 position;
    Vec3 target;     // Point camera is looking at
    Vec3 up;         // Up direction (usually world Y+)

    // First-person camera orientation (Euler angles)
    // Using explicit yaw/pitch prevents drift from incremental rotations
    float yaw;       // Horizontal rotation (radians) - 0 = looking along -Z
    float pitch;     // Vertical rotation (radians) - 0 = level, ±PI/2 = straight up/down

    // Projection parameters
    float fov;       // Field of view (radians)
    float aspect;    // Aspect ratio (width/height)
    float nearPlane; // Near clipping plane
    float farPlane;  // Far clipping plane

    // Cached matrices (updated when camera moves)
    Mat4 viewMatrix;
    Mat4 projectionMatrix;
    bool viewDirty;
    bool projectionDirty;

public:
    // ==========================================================================
    // CONSTRUCTOR
    // ==========================================================================
    Camera(const Vec3& position = Vec3(0, 0, 5),
           const Vec3& target = Vec3(0, 0, 0),
           const Vec3& up = Vec3(0, 1, 0),
           float fovDegrees = 60.0f,
           float aspect = 16.0f / 9.0f,
           float near = 0.1f,
           float far = 100.0f)
        : position(position)
        , target(target)
        , up(up)
        , fov(fovDegrees * M_PI / 180.0f)  // Convert degrees to radians
        , aspect(aspect)
        , nearPlane(near)
        , farPlane(far)
        , viewDirty(true)
        , projectionDirty(true)
    {
        // Calculate initial yaw and pitch from position and target
        Vec3 direction = (target - position).normalized();
        yaw = std::atan2(direction.x, -direction.z);  // atan2(x, -z) for OpenGL coords
        pitch = std::asin(direction.y);               // asin(y) for pitch
    }

    // ==========================================================================
    // GETTERS
    // ==========================================================================
    Vec3 getPosition() const { return position; }
    Vec3 getTarget() const { return target; }
    Vec3 getUp() const { return up; }

    // Get camera's local axes
    Vec3 getForward() const {
        return (target - position).normalized();
    }

    Vec3 getRight() const {
        return getForward().cross(up).normalized();
    }

    Vec3 getUpVector() const {
        return getRight().cross(getForward()).normalized();
    }

    // ==========================================================================
    // SETTERS (mark matrices as dirty for lazy evaluation)
    // ==========================================================================
    void setPosition(const Vec3& pos) {
        position = pos;
        viewDirty = true;
    }

    void setTarget(const Vec3& tgt) {
        target = tgt;
        viewDirty = true;
    }

    void setUp(const Vec3& u) {
        up = u;
        viewDirty = true;
    }

    void setFOV(float fovDegrees) {
        fov = fovDegrees * M_PI / 180.0f;
        projectionDirty = true;
    }

    void setAspectRatio(float aspectRatio) {
        aspect = aspectRatio;
        projectionDirty = true;
    }

    void setClipPlanes(float near, float far) {
        nearPlane = near;
        farPlane = far;
        projectionDirty = true;
    }

    // ==========================================================================
    // CAMERA MOVEMENT
    // ==========================================================================

    // Move camera forward/backward along view direction
    void moveForward(float distance) {
        Vec3 forward = getForward();
        position += forward * distance;
        target += forward * distance;
        viewDirty = true;
    }

    // Strafe left/right
    void moveRight(float distance) {
        Vec3 right = getRight();
        position += right * distance;
        target += right * distance;
        viewDirty = true;
    }

    // Move up/down (world space)
    void moveUp(float distance) {
        position += up * distance;
        target += up * distance;
        viewDirty = true;
    }

    // ==========================================================================
    // CAMERA ROTATION
    // ==========================================================================

    // Orbit around target point
    // Used for: object viewer, rotating around a model
    void orbitAroundTarget(float yawRadians, float pitchRadians) {
        // Vector from target to camera
        Vec3 offset = position - target;
        float radius = offset.length();

        // Convert to spherical coordinates
        float theta = std::atan2(offset.x, offset.z);  // Yaw
        float phi = std::acos(offset.y / radius);      // Pitch

        // Apply rotation
        theta += yawRadians;
        phi = std::fmax(0.1f, std::fmin(M_PI - 0.1f, phi + pitchRadians));

        // Convert back to Cartesian
        offset.x = radius * std::sin(phi) * std::sin(theta);
        offset.y = radius * std::cos(phi);
        offset.z = radius * std::cos(phi) * std::cos(theta);

        position = target + offset;
        viewDirty = true;
    }

    // Look around (first-person camera)
    // Changes target based on camera position
    void lookAround(float yawRadians, float pitchRadians) {
        Vec3 forward = getForward();

        // Build rotation matrices
        Mat4 yawRotation = Mat4::rotateY(yawRadians);
        Mat4 pitchRotation = Mat4::rotateX(pitchRadians);

        // Apply rotations to forward vector
        forward = (yawRotation * pitchRotation).transformDirection(forward);

        target = position + forward;
        viewDirty = true;
    }

    // ==========================================================================
    // FIRST-PERSON ROTATION (using explicit yaw/pitch tracking)
    // Prevents drift that can occur with incremental rotations
    // ==========================================================================

    // Rotate camera horizontally (yaw)
    // Positive = rotate right, Negative = rotate left
    void rotateYaw(float deltaRadians) {
        yaw += deltaRadians;

        // Normalize yaw to [-PI, PI] to prevent overflow
        while (yaw > M_PI) yaw -= 2.0f * M_PI;
        while (yaw < -M_PI) yaw += 2.0f * M_PI;

        updateTargetFromOrientation();
    }

    // Rotate camera vertically (pitch)
    // Positive = look up, Negative = look down
    void rotatePitch(float deltaRadians) {
        pitch += deltaRadians;

        // Clamp pitch to prevent gimbal lock and camera flipping
        // -89° to +89° (slightly less than 90° to avoid singularity)
        const float maxPitch = 89.0f * M_PI / 180.0f;  // ~1.553 radians
        pitch = std::fmax(-maxPitch, std::fmin(maxPitch, pitch));

        updateTargetFromOrientation();
    }

private:
    // ==========================================================================
    // HELPER: Update target from yaw/pitch
    // Converts spherical coordinates (yaw, pitch) to Cartesian direction
    // ==========================================================================
    void updateTargetFromOrientation() {
        // Calculate direction vector from yaw and pitch (spherical coordinates)
        // Using OpenGL coordinate system: -Z is forward, +Y is up, +X is right
        Vec3 direction;
        direction.x = std::cos(pitch) * std::sin(yaw);
        direction.y = std::sin(pitch);
        direction.z = -std::cos(pitch) * std::cos(yaw);  // -Z for OpenGL forward

        // Update target to be 1 unit in front of camera
        target = position + direction;
        viewDirty = true;
    }

public:

    // ==========================================================================
    // MATRIX GETTERS (lazy evaluation for performance)
    // ==========================================================================
    const Mat4& getViewMatrix() {
        if (viewDirty) {
            viewMatrix = Mat4::lookAt(position, target, up);
            viewDirty = false;
        }
        return viewMatrix;
    }

    const Mat4& getProjectionMatrix() {
        if (projectionDirty) {
            projectionMatrix = Mat4::perspective(fov, aspect, nearPlane, farPlane);
            projectionDirty = false;
        }
        return projectionMatrix;
    }

    // ==========================================================================
    // COMBINED VIEW-PROJECTION MATRIX
    // Common optimization: combine matrices once instead of per-vertex
    // ==========================================================================
    Mat4 getViewProjectionMatrix() {
        return getProjectionMatrix() * getViewMatrix();
    }

    // ==========================================================================
    // SCREEN-SPACE PROJECTION
    // Convert a 3D world point to 2D screen coordinates
    // Returns Vec3 with (screenX, screenY, depth)
    // ==========================================================================
    Vec3 worldToScreen(const Vec3& worldPos, int screenWidth, int screenHeight) const {
        // Apply view and projection transformations
        Mat4 vp = projectionMatrix * viewMatrix;
        Vec4 clipSpace = vp * Vec4(worldPos, 1.0f);

        // Perspective divide → normalized device coordinates (NDC)
        Vec3 ndc = clipSpace.toVec3();

        // NDC is in range [-1, 1]. Convert to screen pixels [0, width/height]
        float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
        float screenY = (1.0f - ndc.y) * 0.5f * screenHeight;  // Flip Y (screen Y goes down)

        return Vec3(screenX, screenY, ndc.z);
    }
};
