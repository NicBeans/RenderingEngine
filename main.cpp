#include "WindowGL.h"
#include "RendererGL.h"
#include "Mesh.h"
#include "Camera.h"
#include "Mat4.h"
#include "Vec3.h"
#include <iostream>
#include <cmath>
#include <array>

int main() {
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    try {
        // ======================================================================
        // INITIALIZATION
        // ======================================================================
        WindowGL window("3D Renderer --- ESC to quit",
                        WINDOW_WIDTH, WINDOW_HEIGHT);

        RendererGL renderer;  // OpenGL renderer (no framebuffer needed!)

        // ======================================================================
        // CREATE CAMERA
        // Position: (0, 2, 8) - above and back from origin
        // Target: (0, 0, 0) - looking at world center
        // Up: (0, 1, 0) - Y is up
        // FOV: 60 degrees
        // Aspect: 800/600 = 1.333
        // ======================================================================
        Camera camera(
            Vec3(0, 2, -8),     // Eye position
            Vec3(0, 0, 0),     // Look at origin
            Vec3(0, 1, 0),     // Up vector
            90.0f,             // Field of view (degrees)
            static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT,  // Aspect ratio
            0.1f,              // Near plane
            100.0f             // Far plane
        );

        // ======================================================================
        // CREATE 3D MESHES
        // We'll create multiple objects with different colors and positions
        // Using more saturated, interesting colors that show lighting better
        // ======================================================================

        // Letter N segments (reuse scaled cubes for each bar of the glyph)
        Mesh letterBar = Mesh::createCube(1.0f, Color(uint8_t{40}, uint8_t{190}, uint8_t{255}));

        // Corner cube (CC) pieces (base + two walls to mock a room corner)
        Mesh ccFloor = Mesh::createCube(1.0f, Color(uint8_t{160}, uint8_t{160}, uint8_t{160}));
        Mesh ccWallX = Mesh::createCube(1.0f, Color(uint8_t{190}, uint8_t{190}, uint8_t{190}));
        Mesh ccWallZ = Mesh::createCube(1.0f, Color(uint8_t{190}, uint8_t{190}, uint8_t{190}));

        // Render both sides so walls/floor remain opaque from every viewing angle
        ccFloor.makeDoubleSided();
        ccWallX.makeDoubleSided();
        ccWallZ.makeDoubleSided();

        // ======================================================================
        // LIGHT SOURCE VISUALIZATION
        // Create a small bright sphere to show where light is coming from
        // Light direction: (-0.45, 0.82, -0.4) normalized (aims down + into the corner)
        // ======================================================================
        Vec3 lightDirection = Vec3(-0.45f, 0.82f, -0.4f).normalized();
        Mesh lightSource = Mesh::createSphere(0.3f, 10, 10, Color(uint8_t{255}, uint8_t{255}, uint8_t{200}));

        std::cout << "=== Renderer ===" << std::endl;
        std::cout << "Resolution: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
        std::cout << "Meshes loaded:" << std::endl;
        std::cout << "  Letter-N bar mesh: " << letterBar.getTriangleCount() << " triangles" << std::endl;
        std::cout << "  CC floor: " << ccFloor.getTriangleCount() << " triangles" << std::endl;
        std::cout << "  CC wall (X): " << ccWallX.getTriangleCount() << " triangles" << std::endl;
        std::cout << "  CC wall (Z): " << ccWallZ.getTriangleCount() << " triangles" << std::endl;
        std::cout << "\nControls:" << std::endl;
        std::cout << "  W/A/S/D: Move camera (forward/left/back/right)" << std::endl;
        std::cout << "  Arrow Keys: Look around (rotate view)" << std::endl;
        std::cout << "  Q/E: Move up/down" << std::endl;
        std::cout << "  ESC: Quit" << std::endl;

        // ======================================================================
        // TIME STEP & MOVEMENT SPEEDS
        // ======================================================================
        float time = 0.0f;
        const float dt = 1.0f / 60.0f;  // 60 FPS timestep

        // Camera movement speed
        const float moveSpeed = 3.0f * dt;

        // Camera rotation speed (radians per frame)
        const float rotateSpeed = 2.0f * M_PI / 180.0f * dt * 60.0f;  // ~2 degrees per frame at 60 FPS

        // ======================================================================
        // FPS TRACKING
        // ======================================================================
        Uint32 lastTime = SDL_GetTicks();  // Milliseconds since SDL init
        int frameCount = 0;
        float fps = 0.0f;
        const float FPS_UPDATE_INTERVAL = 0.5f;  // Update FPS display every 0.5s
        float fpsTimer = 0.0f;

        // ======================================================================
        // MAIN LOOP - THE HEART OF REAL-TIME RENDERING
        // ======================================================================
        while (window.pollEvents()) {
            // ==================================================================
            // INPUT HANDLING
            // Full 6-DOF camera controls (move + look)
            // ==================================================================
            const Uint8* keystate = SDL_GetKeyboardState(nullptr);

            // WASD: Move camera position
            if (keystate[SDL_SCANCODE_W]) {
                camera.moveForward(moveSpeed);
            }
            if (keystate[SDL_SCANCODE_S]) {
                camera.moveForward(-moveSpeed);
            }
            if (keystate[SDL_SCANCODE_A]) {
                camera.moveRight(-moveSpeed);
            }
            if (keystate[SDL_SCANCODE_D]) {
                camera.moveRight(moveSpeed);
            }

            // Q/E: Move up/down (vertical)
            if (keystate[SDL_SCANCODE_Q]) {
                camera.moveUp(moveSpeed);
            }
            if (keystate[SDL_SCANCODE_E]) {
                camera.moveUp(-moveSpeed);
            }

            // Arrow keys: Rotate view (look around)
            if (keystate[SDL_SCANCODE_LEFT]) {
                camera.rotateYaw(-rotateSpeed);  // Look left
            }
            if (keystate[SDL_SCANCODE_RIGHT]) {
                camera.rotateYaw(rotateSpeed);  // Look right
            }
            if (keystate[SDL_SCANCODE_UP]) {
                camera.rotatePitch(rotateSpeed);  // Look up
            }
            if (keystate[SDL_SCANCODE_DOWN]) {
                camera.rotatePitch(-rotateSpeed);  // Look down
            }

            // ==================================================================
            // LIGHT SPACE MATRIX
            // Calculate view and projection matrices from light's perspective
            // ==================================================================

            // Light position (far away, directional light like the sun)
            float lightDistance = 15.0f;
            Vec3 lightPos = lightDirection * lightDistance;

            // View matrix from light's perspective (looking at scene center)
            Mat4 lightView = Mat4::lookAt(
                lightPos,           // Light position
                Vec3(0, 0, 0),      // Look at scene center
                Vec3(0, 1, 0)       // Up vector
            );

            // Orthographic projection for directional light
            // Covers area where shadows can be cast
            float shadowArea = 15.0f;
            Mat4 lightProjection = Mat4::ortho(
                -shadowArea, shadowArea,    // Left, right
                -shadowArea, shadowArea,    // Bottom, top
                0.1f, 50.0f                 // Near, far
            );

            // Combined light space matrix (projection * view)
            Mat4 lightSpaceMatrix = lightProjection * lightView;

            // ==================================================================
            // BUILD TRANSFORMATION MATRICES
            // Model Matrix = Translate * Rotate * Scale
            // Order matters! Scale first, then rotate, then translate
            // ==================================================================

            // LETTER N ROOT - Shared transform for all three bars of the glyph
            Mat4 letterRoot = Mat4::translate(1.5f, 1.0f, 1.5f)      // Between light source and CC corner
                             * Mat4::rotateY(time * 0.6f)            // Steady spin (≈34°/s)
                             * Mat4::rotateX(0.35f);                 // Slight tilt for depth readability

            const float legHeight = 2.75f;
            const float legThickness = 0.4f;
            const float legDepth = 0.6f;
            const float legOffsetX = 0.85f;
            const float innerSpanX = 2.0f * (legOffsetX - legThickness * 0.5f);
            const float diagonalLength = std::sqrt(innerSpanX * innerSpanX + legHeight * legHeight) + legThickness;
            const float diagonalAngle = -std::atan2(innerSpanX, legHeight);

            // Build local transforms for each bar (left vertical, right vertical, diagonal)
            std::array<Mat4, 3> letterSegments = {
                letterRoot * (Mat4::translate(-legOffsetX, 0.0f, 0.0f)
                              * Mat4::scale(legThickness, legHeight, legDepth)),
                letterRoot * (Mat4::translate(legOffsetX, 0.0f, 0.0f)
                              * Mat4::scale(legThickness, legHeight, legDepth)),
                letterRoot * (Mat4::rotateZ(diagonalAngle)
                              * Mat4::scale(legThickness, diagonalLength, legDepth))
            };

            // CC ROOT - 180° spin keeps the corner behind the glyph relative to the light
            const Mat4 ccTurn = Mat4::rotateY(static_cast<float>(M_PI));

            // CC FLOOR - Light gray platform catching the shadow footprint
            Mat4 floorModel = Mat4::translate(1.5f, -0.7f, 1.5f)     // Inner corner sits directly beneath the glyph (≈1.5, 0, 1.5)
                             * ccTurn
                             * Mat4::scale(6.0f, 0.2f, 6.0f);        // Wide, thin slab

            // CC WALLS - Two panels forming the L-shaped backdrop
            Mat4 wallXModel = Mat4::translate(4.5f, 0.7f, 1.5f)      // X-side wall hugs the glyph's left edge
                             * ccTurn
                             * Mat4::scale(0.2f, 3.0f, 6.0f);        // Tall along Y, deep along Z

            Mat4 wallZModel = Mat4::translate(1.5f, 0.7f, 4.5f)      // Z-side wall closes the corner behind the glyph
                             * ccTurn
                             * Mat4::scale(6.0f, 3.0f, 0.2f);        // Mirror layout

            // LIGHT SOURCE - Position far away in light direction
            Mat4 lightModel = Mat4::translate(lightPos.x, lightPos.y, lightPos.z)
                            * Mat4::scale(0.5f);  // Small but visible

            // ==================================================================
            // SHADOW PASS (PASS 1)
            // Render scene from light's perspective to build shadow map
            // ==================================================================
            renderer.beginShadowPass();

            // Render all shadow-casting objects
            renderer.renderShadowMesh(ccFloor, floorModel, lightSpaceMatrix);
            renderer.renderShadowMesh(ccWallX, wallXModel, lightSpaceMatrix);
            renderer.renderShadowMesh(ccWallZ, wallZModel, lightSpaceMatrix);
            for (const Mat4& segment : letterSegments) {
                renderer.renderShadowMesh(letterBar, segment, lightSpaceMatrix);
            }
            // Don't render light source to shadow map (it's emissive)

            renderer.endShadowPass(WINDOW_WIDTH, WINDOW_HEIGHT);

            // ==================================================================
            // NORMAL RENDERING PASS (PASS 2)
            // Render scene from camera's perspective, using shadow map
            // ==================================================================
            window.clear();  // Clear screen for normal rendering

            // Draw corner environment
            renderer.drawMesh(ccFloor, floorModel, camera, lightSpaceMatrix);
            renderer.drawMesh(ccWallX, wallXModel, camera, lightSpaceMatrix);
            renderer.drawMesh(ccWallZ, wallZModel, camera, lightSpaceMatrix);

            // Draw spinning letter (after walls so it sits in front)
            for (const Mat4& segment : letterSegments) {
                renderer.drawMesh(letterBar, segment, camera, lightSpaceMatrix);
            }

            // Draw light source (emissive = true, so it glows and isn't affected by lighting)
            renderer.drawMesh(lightSource, lightModel, camera, lightSpaceMatrix, true);

            // ==================================================================
            // FPS COUNTER
            // Display FPS in top-right corner
            // TODO: Re-implement text rendering for OpenGL
            // For now, check terminal output or use GPU profiler
            // ==================================================================
            // Calculate FPS
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            frameCount++;
            fpsTimer += deltaTime;

            // Update FPS display every 0.5 seconds
            if (fpsTimer >= FPS_UPDATE_INTERVAL) {
                fps = frameCount / fpsTimer;
                frameCount = 0;
                fpsTimer = 0.0f;

                // Print FPS to console for now
                std::cout << "FPS: " << static_cast<int>(fps) << std::endl;
            }

            // ==================================================================
            // SWAP BUFFERS
            // This is when frame appears on screen!
            // ==================================================================
            window.swapBuffers();

            // Advance animation clock (drives the spinning letter)
            time += dt;
        }

        std::cout << "\nShutting down..." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// =============================================================================
// WHAT YOU'VE BUILT: A COMPLETE 3D ENGINE!
// =============================================================================
//
// MATHEMATICS:
// - Vec3/Vec4: 3D/4D vectors with dot and cross products
// - Mat4: 4×4 matrices for transformations
// - Homogeneous coordinates (the 4th dimension!)
//
// TRANSFORMATIONS:
// - Translation, Rotation, Scale matrices
// - Matrix multiplication for combining transforms
// - Model, View, Projection matrices (MVP)
//
// CAMERA SYSTEM:
// - LookAt matrix (view transformation)
// - Perspective projection (THE magic of 3D)
// - Camera movement and controls
//
// RENDERING PIPELINE:
// - Vertex transformation (object → world → camera → clip → screen)
// - Perspective division (divide by W)
// - Viewport transformation
// - Rasterization with barycentric coordinates
// - Depth testing (Z-buffer)
// - Backface culling
// - Simple lighting (Lambertian diffuse)
//
// WHAT'S NEXT?
//
// 1. BETTER LIGHTING:
//    - Phong shading (ambient + diffuse + specular)
//    - Multiple lights
//    - Point lights, spotlights
//    - Normal mapping
//
// 2. TEXTURING:
//    - UV coordinates
//    - Texture sampling
//    - Mipmapping
//    - Bilinear/trilinear filtering
//
// 3. ADVANCED RENDERING:
//    - Shadows (shadow mapping)
//    - Reflections (environment mapping)
//    - Transparency (alpha blending)
//    - Post-processing (bloom, HDR, tone mapping)
//
// 4. OPTIMIZATION:
//    - Frustum culling
//    - Occlusion culling
//    - Level of detail (LOD)
//    - Instancing
//
// 5. GPU RENDERING (OpenGL/Vulkan):
//    - Vertex shaders (run on GPU vertices)
//    - Fragment shaders (run on GPU pixels)
//    - Uniform buffers
//    - Texture units
//    - The SAME concepts, but 1000× faster!
//
// 6. GAME ENGINE FEATURES:
//    - Scene graph (hierarchical transforms)
//    - Entity-component system
//    - Physics engine integration
//    - Animation system (skeletal, blend shapes)
//    - Asset loading (OBJ, FBX, GLTF)
//    - Material system
//
// =============================================================================
// CONGRATULATIONS!
// You now understand the fundamentals that power:
// - Game engines (Unity, Unreal, Godot)
// - 3D modeling software (Blender, Maya)
// - CAD software
// - Scientific visualization
// - VR/AR applications
// - Movie rendering (Pixar, Disney)
//
// Everything you learned here applies to GPU programming.
// The GPU does the SAME operations, just massively parallel!
// =============================================================================
