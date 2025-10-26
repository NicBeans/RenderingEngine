// =============================================================================
// 3D RENDERER DEMO - OpenGL GPU Accelerated!
// =============================================================================
// This demonstrates the FULL 3D rendering pipeline:
// - 3D transformations (translate, rotate, scale)
// - Camera system (view and projection matrices)
// - Perspective projection (3D → 2D)
// - Z-buffering (depth testing) - IN HARDWARE!
// - Backface culling - IN HARDWARE!
// - Simple lighting (diffuse + ambient) - ON GPU!
// - Multiple meshes in a scene
//
// GPU PIPELINE (all in parallel on thousands of cores):
// Vertex (3D) → Vertex Shader (GPU) → World/Camera/Clip Space
//             → Rasterization (GPU Hardware)
//             → Fragment Shader (GPU) → Lighting + Color
//             → Depth Test (GPU Hardware)
//             → Framebuffer (GPU VRAM)
//             → Display!
//
// CPU ONLY DOES:
// - Update matrices (16 floats/mesh/frame)
// - Issue draw calls (1 per mesh)
// - Handle input
//
// RESULT: 100-1000× faster than software renderer!
// =============================================================================

#include "WindowGL.h"
#include "RendererGL.h"
#include "Mesh.h"
#include "Camera.h"
#include "Mat4.h"
#include "Vec3.h"
#include <iostream>
#include <cmath>

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
        // Position: looking into the corner from outside
        // Target: center of the corner area where N will be
        // Up: (0, 1, 0) - Y is up
        // FOV: 60 degrees
        // Aspect: 800/600 = 1.333
        // ======================================================================
        Camera camera(
            Vec3(5, 3, 5),     // Eye position - outside the corner, elevated
            Vec3(1, 1.5, 1),   // Look at where the N will be
            Vec3(0, 1, 0),     // Up vector
            75.0f,             // Field of view (degrees)
            static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT,  // Aspect ratio
            0.1f,              // Near plane
            100.0f             // Far plane
        );

        // ======================================================================
        // CREATE 3D MESHES
        // We'll create multiple objects with different colors and positions
        // Using more saturated, interesting colors that show lighting better
        // ======================================================================

        // Letter N (spinning object) - Vibrant cyan/turquoise
        Mesh letterN = Mesh::createLetterN(2.0f, 1.5f, 0.2f, Color(uint8_t{0}, uint8_t{200}, uint8_t{255}));

        // Corner cube (CC) - replaces ground - Bright white/reflective for better shadow visibility
        Mesh cornerCube = Mesh::createCornerCube(10.0f, Color(uint8_t{240}, uint8_t{240}, uint8_t{240}));

        // ======================================================================
        // LIGHT SOURCE VISUALIZATION
        // Create a small bright sphere to show where light is coming from
        // Light direction: (0.3, 0.8, 0.5) normalized
        // ======================================================================
        Vec3 lightDirection = Vec3(0.3f, 0.8f, 0.5f).normalized();
        Mesh lightSource = Mesh::createSphere(0.3f, 10, 10, Color(uint8_t{255}, uint8_t{255}, uint8_t{200}));

        std::cout << "=== Renderer ===" << std::endl;
        std::cout << "Resolution: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
        std::cout << "Meshes loaded:" << std::endl;
        std::cout << "  Letter N: " << letterN.getTriangleCount() << " triangles" << std::endl;
        std::cout << "  Corner Cube: " << cornerCube.getTriangleCount() << " triangles" << std::endl;
        std::cout << "\nControls:" << std::endl;
        std::cout << "  W/A/S/D: Move camera (forward/left/back/right)" << std::endl;
        std::cout << "  Arrow Keys: Look around (rotate view)" << std::endl;
        std::cout << "  Q/E: Move up/down" << std::endl;
        std::cout << "  ESC: Quit" << std::endl;

        // ======================================================================
        // ANIMATION STATE
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

            // LETTER N - Spinning around Y axis, positioned in the corner
            Mat4 letterNModel = Mat4::translate(1.5, 1.0, 1.5)  // Position in the corner (floor at y=0, walls at x=0 and z=0)
                              * Mat4::rotateY(time * 0.8f)      // Spin around Y axis
                              * Mat4::scale(0.8f);              // Slightly smaller for better fit

            // CORNER CUBE (CC) - Static at origin
            Mat4 cornerCubeModel = Mat4::translate(0, 0, 0)     // At origin (corner at 0,0,0)
                                 * Mat4::scale(1.0f);           // Normal size

            // LIGHT SOURCE - Position far away in light direction
            Mat4 lightModel = Mat4::translate(lightPos.x, lightPos.y, lightPos.z)
                            * Mat4::scale(0.5f);  // Small but visible

            // ==================================================================
            // SHADOW PASS (PASS 1)
            // Render scene from light's perspective to build shadow map
            // ==================================================================
            renderer.beginShadowPass();

            // Render all shadow-casting objects
            renderer.renderShadowMesh(cornerCube, cornerCubeModel, lightSpaceMatrix);
            renderer.renderShadowMesh(letterN, letterNModel, lightSpaceMatrix);
            // Don't render light source to shadow map (it's emissive)

            renderer.endShadowPass(WINDOW_WIDTH, WINDOW_HEIGHT);

            // ==================================================================
            // NORMAL RENDERING PASS (PASS 2)
            // Render scene from camera's perspective, using shadow map
            // ==================================================================
            window.clear();  // Clear screen for normal rendering

            // Draw corner cube first
            renderer.drawMesh(cornerCube, cornerCubeModel, camera, lightSpaceMatrix);

            // Draw letter N (main object that casts shadow)
            renderer.drawMesh(letterN, letterNModel, camera, lightSpaceMatrix);

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

            // ==================================================================
            // UPDATE TIME
            // ==================================================================
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
