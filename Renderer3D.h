#pragma once
#include "Renderer.h"
#include "Framebuffer.h"
#include "Mesh.h"
#include "Camera.h"
#include "Mat4.h"
#include "Vec3.h"
#include <algorithm>

// =============================================================================
// Renderer3D: 3D rendering on top of 2D renderer
// =============================================================================
// THE 3D RENDERING PIPELINE:
//
// 1. VERTEX PROCESSING:
//    - Transform vertices: Model → World → View → Clip space
//    - Apply MVP matrix (Model-View-Projection)
//
// 2. PERSPECTIVE DIVISION:
//    - Divide by w: (x/w, y/w, z/w) → Normalized Device Coordinates (NDC)
//    - This makes distant objects smaller!
//
// 3. VIEWPORT TRANSFORM:
//    - Convert NDC [-1,1] to screen pixels [0, width/height]
//
// 4. RASTERIZATION:
//    - Fill triangles pixel-by-pixel with depth testing
//
// 5. FRAGMENT PROCESSING:
//    - Lighting, texturing, shading (we'll do simple flat shading)
//
// This mirrors what GPUs do, but on CPU!
// =============================================================================

class Renderer3D : public Renderer {
public:
    explicit Renderer3D(Framebuffer& fb) : Renderer(fb) {}

    // ==========================================================================
    // DRAW 3D MESH
    // The main function for 3D rendering!
    //
    // Parameters:
    // - mesh: 3D geometry (vertices, triangles)
    // - modelMatrix: object's position/rotation/scale in world
    // - camera: view and projection matrices
    // - wireframe: draw only edges (debugging) vs filled triangles
    // ==========================================================================
    void drawMesh(const Mesh& mesh,
                  const Mat4& modelMatrix,
                  Camera& camera,
                  bool wireframe = false) {

        Mat4 view = camera.getViewMatrix();
        Mat4 projection = camera.getProjectionMatrix();

        // Combined MVP matrix
        // Order matters! projection * view * model
        Mat4 mvp = projection * view * modelMatrix;

        // Process each triangle
        for (size_t i = 0; i < mesh.getTriangleCount(); i++) {
            Vertex v0, v1, v2;
            mesh.getTriangle(i, v0, v1, v2);

            // ================================================================
            // VERTEX PROCESSING
            // Transform vertices from object space → clip space
            // ================================================================
            Vec4 clipV0 = mvp * Vec4(v0.position, 1.0f);
            Vec4 clipV1 = mvp * Vec4(v1.position, 1.0f);
            Vec4 clipV2 = mvp * Vec4(v2.position, 1.0f);

            // ================================================================
            // CLIPPING (SIMPLIFIED)
            // In real renderer, we'd clip triangles against view frustum
            // For now, just reject if all vertices out of bounds
            // ================================================================
            // Skip if entirely behind camera (w <= 0)
            if (clipV0.w <= 0 && clipV1.w <= 0 && clipV2.w <= 0) continue;

            // ================================================================
            // PERSPECTIVE DIVISION
            // Divide by w to get Normalized Device Coordinates (NDC)
            // NDC range: [-1, 1] in all axes
            // ================================================================
            Vec3 ndcV0 = clipV0.toVec3();
            Vec3 ndcV1 = clipV1.toVec3();
            Vec3 ndcV2 = clipV2.toVec3();

            // ================================================================
            // VIEWPORT TRANSFORMATION
            // Convert NDC [-1,1] to screen coordinates [0, width/height]
            // Y is flipped: NDC +Y is up, screen +Y is down
            // ================================================================
            int width = framebuffer.getWidth();
            int height = framebuffer.getHeight();

            Vec2 screenV0(
                (ndcV0.x + 1.0f) * 0.5f * width,
                (1.0f - ndcV0.y) * 0.5f * height  // Flip Y
            );
            Vec2 screenV1(
                (ndcV1.x + 1.0f) * 0.5f * width,
                (1.0f - ndcV1.y) * 0.5f * height
            );
            Vec2 screenV2(
                (ndcV2.x + 1.0f) * 0.5f * width,
                (1.0f - ndcV2.y) * 0.5f * height
            );

            // Depth values (z in NDC is already normalized to [0,1])
            float depth0 = ndcV0.z;
            float depth1 = ndcV1.z;
            float depth2 = ndcV2.z;

            // ================================================================
            // BACKFACE CULLING
            // Don't draw triangles facing away from camera
            // Check winding order: if clockwise on screen, it's facing away
            // ================================================================
            Vec2 edge1 = screenV1 - screenV0;
            Vec2 edge2 = screenV2 - screenV0;
            float cross = edge1.cross(edge2);

            if (cross <= 0) {
                continue;  // Back-facing, skip!
            }

            // ================================================================
            // LIGHTING CALCULATION
            // CRITICAL: Transform normal from object space to world space!
            // ================================================================

            // Calculate triangle normal in object space
            Vec3 objectNormal = calculateTriangleNormal(v0.position, v1.position, v2.position);

            // Transform normal to world space using model matrix
            // Use transformDirection (w=0) so translation doesn't affect it
            Vec3 worldNormal = modelMatrix.transformDirection(objectNormal).normalized();

            // Light direction (world space) - coming from top-right-front
            Vec3 lightDir = Vec3(0.3f, 0.8f, 0.5f).normalized();

            // Calculate brightness using Lambertian diffuse model
            // dot(normal, light) = cos(angle) = brightness
            float diffuse = std::max(0.0f, worldNormal.dot(lightDir));

            // Add ambient light so nothing is completely black
            float ambient = 0.3f;  // 30% ambient illumination
            float brightness = ambient + (1.0f - ambient) * diffuse;

            // Clamp to [0, 1] range
            brightness = std::min(1.0f, std::max(0.0f, brightness));

            // Average the vertex colors for this triangle
            Color baseColor(
                uint8_t((v0.color.r + v1.color.r + v2.color.r) / 3),
                uint8_t((v0.color.g + v1.color.g + v2.color.g) / 3),
                uint8_t((v0.color.b + v1.color.b + v2.color.b) / 3),
                uint8_t(255)
            );

            // Apply lighting to color
            Color litColor(
                uint8_t(baseColor.r * brightness),
                uint8_t(baseColor.g * brightness),
                uint8_t(baseColor.b * brightness),
                baseColor.a
            );

            // ================================================================
            // RASTERIZATION
            // ================================================================
            if (wireframe) {
                // Draw edges only
                drawLine(screenV0, screenV1, litColor);
                drawLine(screenV1, screenV2, litColor);
                drawLine(screenV2, screenV0, litColor);
            } else {
                // Draw filled triangle with depth testing
                drawTriangle3D(screenV0, screenV1, screenV2,
                               depth0, depth1, depth2,
                               litColor);
            }
        }
    }

private:
    // ==========================================================================
    // CALCULATE TRIANGLE NORMAL
    // Normal = edge1 × edge2 (cross product)
    // Used for lighting calculations
    // ==========================================================================
    Vec3 calculateTriangleNormal(const Vec3& v0, const Vec3& v1, const Vec3& v2) {
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        return edge1.cross(edge2).normalized();
    }

    // ==========================================================================
    // DRAW 3D TRIANGLE WITH DEPTH TESTING
    // Rasterizes triangle with proper depth interpolation
    // ==========================================================================
    void drawTriangle3D(const Vec2& v0, const Vec2& v1, const Vec2& v2,
                        float depth0, float depth1, float depth2,
                        const Color& color) {

        // Get bounding box
        int minX = static_cast<int>(std::max(0.0f, std::min({v0.x, v1.x, v2.x})));
        int maxX = static_cast<int>(std::min(float(framebuffer.getWidth() - 1),
                                              std::max({v0.x, v1.x, v2.x})));
        int minY = static_cast<int>(std::max(0.0f, std::min({v0.y, v1.y, v2.y})));
        int maxY = static_cast<int>(std::min(float(framebuffer.getHeight() - 1),
                                              std::max({v0.y, v1.y, v2.y})));

        // Rasterize: test each pixel in bounding box
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                Vec2 p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);

                // ============================================================
                // BARYCENTRIC COORDINATES
                // Express point p as weighted sum of triangle vertices
                // p = w0*v0 + w1*v1 + w2*v2 where w0+w1+w2 = 1
                //
                // If all weights >= 0, point is inside triangle!
                // Weights also tell us how to interpolate depth, color, etc.
                // ============================================================
                float area = (v1 - v0).cross(v2 - v0);
                if (std::abs(area) < 0.0001f) continue;  // Degenerate triangle

                float w0 = ((v1 - p).cross(v2 - p)) / area;
                float w1 = ((v2 - p).cross(v0 - p)) / area;
                float w2 = ((v0 - p).cross(v1 - p)) / area;

                // Inside triangle?
                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    // ========================================================
                    // DEPTH INTERPOLATION
                    // Interpolate depth using barycentric coordinates
                    // This ensures correct depth for EVERY pixel in triangle!
                    // ========================================================
                    float depth = w0 * depth0 + w1 * depth1 + w2 * depth2;

                    // ========================================================
                    // DEPTH TEST AND DRAW
                    // Only draw if this pixel is closer than what's there
                    // ========================================================
                    framebuffer.setPixelDepth(x, y, depth, color);
                }
            }
        }
    }
};
