#pragma once
#include "Vec3.h"
#include "Color.h"
#include <vector>

// =============================================================================
// Vertex: A single point in 3D space with attributes
// =============================================================================
// In real 3D rendering, vertices have many attributes:
// - position: where in 3D space
// - normal: surface orientation (for lighting)
// - color: vertex color
// - texCoord: texture mapping coordinates (UV)
// - tangent/bitangent: for normal mapping
//
// We'll keep it simple: position, normal, color
// =============================================================================

struct Vertex {
    Vec3 position;
    Vec3 normal;   // Surface normal for lighting
    Color color;   // Per-vertex color

    Vertex() : position(), normal(0, 1, 0), color(Color::WHITE) {}

    Vertex(const Vec3& pos, const Vec3& norm = Vec3(0, 1, 0), const Color& col = Color::WHITE)
        : position(pos), normal(norm), color(col) {}
};

// =============================================================================
// Mesh: Collection of vertices forming 3D geometry
// =============================================================================
// TRIANGLE MESH REPRESENTATION:
// - Vertices: array of Vertex structs
// - Indices: array of integers, 3 per triangle
//
// WHY INDICES?
// Without:  Triangle 1: [v0, v1, v2], Triangle 2: [v2, v1, v3] (6 vertices)
// With:     Vertices: [v0, v1, v2, v3], Indices: [0,1,2, 2,1,3] (4 vertices + 6 indices)
//
// Saves memory! A vertex is ~32 bytes, an index is 4 bytes.
// For a cube: 24 vertices vs 8 vertices + 36 indices = 67% memory reduction!
//
// WINDING ORDER (CRITICAL!):
// Vertices must be in counter-clockwise order when viewed from front
// Used for backface culling: don't draw triangles facing away
// =============================================================================

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;  // Triplets: each 3 indices = 1 triangle

    Mesh() = default;

    // ==========================================================================
    // UTILITY FUNCTIONS
    // ==========================================================================
    size_t getTriangleCount() const {
        return indices.size() / 3;
    }

    // Get vertices of a specific triangle
    void getTriangle(size_t triangleIndex, Vertex& v0, Vertex& v1, Vertex& v2) const {
        size_t idx = triangleIndex * 3;
        v0 = vertices[indices[idx + 0]];
        v1 = vertices[indices[idx + 1]];
        v2 = vertices[indices[idx + 2]];
    }

    // ==========================================================================
    // PRIMITIVE GENERATORS
    // These create basic 3D shapes - building blocks of 3D graphics!
    // ==========================================================================

    // ==========================================================================
    // CREATE CUBE
    // The simplest 3D shape: 8 vertices, 6 faces, 12 triangles
    //
    // Cube layout:
    //     6-------7
    //    /|      /|
    //   2-------3 |
    //   | 4-----|-5
    //   |/      |/
    //   0-------1
    // ==========================================================================
    static Mesh createCube(float size = 1.0f, const Color& color = Color::WHITE) {
        Mesh mesh;
        float half = size * 0.5f;

        // 8 corner vertices
        Vec3 corners[8] = {
            Vec3(-half, -half, -half),  // 0
            Vec3( half, -half, -half),  // 1
            Vec3(-half,  half, -half),  // 2
            Vec3( half,  half, -half),  // 3
            Vec3(-half, -half,  half),  // 4
            Vec3( half, -half,  half),  // 5
            Vec3(-half,  half,  half),  // 6
            Vec3( half,  half,  half)   // 7
        };

        // We need 24 vertices (4 per face) because normals differ per face
        // Front face (z-)
        Vec3 frontNormal(0, 0, -1);
        mesh.vertices.push_back(Vertex(corners[0], frontNormal, color));
        mesh.vertices.push_back(Vertex(corners[1], frontNormal, color));
        mesh.vertices.push_back(Vertex(corners[3], frontNormal, color));
        mesh.vertices.push_back(Vertex(corners[2], frontNormal, color));

        // Back face (z+)
        Vec3 backNormal(0, 0, 1);
        mesh.vertices.push_back(Vertex(corners[5], backNormal, color));
        mesh.vertices.push_back(Vertex(corners[4], backNormal, color));
        mesh.vertices.push_back(Vertex(corners[6], backNormal, color));
        mesh.vertices.push_back(Vertex(corners[7], backNormal, color));

        // Left face (x-)
        Vec3 leftNormal(-1, 0, 0);
        mesh.vertices.push_back(Vertex(corners[4], leftNormal, color));
        mesh.vertices.push_back(Vertex(corners[0], leftNormal, color));
        mesh.vertices.push_back(Vertex(corners[2], leftNormal, color));
        mesh.vertices.push_back(Vertex(corners[6], leftNormal, color));

        // Right face (x+)
        Vec3 rightNormal(1, 0, 0);
        mesh.vertices.push_back(Vertex(corners[1], rightNormal, color));
        mesh.vertices.push_back(Vertex(corners[5], rightNormal, color));
        mesh.vertices.push_back(Vertex(corners[7], rightNormal, color));
        mesh.vertices.push_back(Vertex(corners[3], rightNormal, color));

        // Bottom face (y-)
        Vec3 bottomNormal(0, -1, 0);
        mesh.vertices.push_back(Vertex(corners[4], bottomNormal, color));
        mesh.vertices.push_back(Vertex(corners[5], bottomNormal, color));
        mesh.vertices.push_back(Vertex(corners[1], bottomNormal, color));
        mesh.vertices.push_back(Vertex(corners[0], bottomNormal, color));

        // Top face (y+)
        Vec3 topNormal(0, 1, 0);
        mesh.vertices.push_back(Vertex(corners[2], topNormal, color));
        mesh.vertices.push_back(Vertex(corners[3], topNormal, color));
        mesh.vertices.push_back(Vertex(corners[7], topNormal, color));
        mesh.vertices.push_back(Vertex(corners[6], topNormal, color));

        // Indices (2 triangles per face, counter-clockwise winding)
        for (uint32_t i = 0; i < 6; i++) {
            uint32_t base = i * 4;
            // Triangle 1
            mesh.indices.push_back(base + 0);
            mesh.indices.push_back(base + 1);
            mesh.indices.push_back(base + 2);
            // Triangle 2
            mesh.indices.push_back(base + 0);
            mesh.indices.push_back(base + 2);
            mesh.indices.push_back(base + 3);
        }

        return mesh;
    }

    // ==========================================================================
    // CREATE PYRAMID (4-sided, square base)
    // 5 vertices, 5 faces (1 square bottom + 4 triangular sides)
    // ==========================================================================
    static Mesh createPyramid(float size = 1.0f, const Color& color = Color::WHITE) {
        Mesh mesh;
        float half = size * 0.5f;

        Vec3 apex(0, half, 0);         // Top point
        Vec3 base[4] = {
            Vec3(-half, -half, -half),  // 0
            Vec3( half, -half, -half),  // 1
            Vec3( half, -half,  half),  // 2
            Vec3(-half, -half,  half)   // 3
        };

        // Bottom face (square)
        Vec3 bottomNormal(0, -1, 0);
        for (int i = 0; i < 4; i++) {
            mesh.vertices.push_back(Vertex(base[i], bottomNormal, color));
        }

        // Side faces (4 triangles)
        // Each triangle: 2 base vertices + apex
        for (int i = 0; i < 4; i++) {
            int next = (i + 1) % 4;
            Vec3 edge1 = base[next] - base[i];
            Vec3 edge2 = apex - base[i];
            Vec3 normal = edge1.cross(edge2).normalized();

            mesh.vertices.push_back(Vertex(base[i], normal, color));
            mesh.vertices.push_back(Vertex(base[next], normal, color));
            mesh.vertices.push_back(Vertex(apex, normal, color));
        }

        // Indices
        // Bottom (2 triangles)
        mesh.indices.push_back(0); mesh.indices.push_back(1); mesh.indices.push_back(2);
        mesh.indices.push_back(0); mesh.indices.push_back(2); mesh.indices.push_back(3);

        // Sides (4 triangles)
        for (uint32_t i = 0; i < 4; i++) {
            uint32_t base_idx = 4 + i * 3;
            mesh.indices.push_back(base_idx + 0);
            mesh.indices.push_back(base_idx + 1);
            mesh.indices.push_back(base_idx + 2);
        }

        return mesh;
    }

    // ==========================================================================
    // CREATE SPHERE (UV sphere - latitude/longitude)
    // Parameters:
    // - radius: sphere size
    // - segments: number of vertical divisions (latitude)
    // - rings: number of horizontal divisions (longitude)
    //
    // More segments/rings = smoother sphere but more triangles
    // ==========================================================================
    static Mesh createSphere(float radius = 1.0f, int segments = 16, int rings = 16,
                             const Color& color = Color::WHITE) {
        Mesh mesh;

        // Generate vertices
        for (int ring = 0; ring <= rings; ring++) {
            float phi = M_PI * float(ring) / float(rings);  // 0 to PI (top to bottom)
            float y = radius * std::cos(phi);
            float ringRadius = radius * std::sin(phi);

            for (int seg = 0; seg <= segments; seg++) {
                float theta = 2.0f * M_PI * float(seg) / float(segments);  // 0 to 2PI
                float x = ringRadius * std::cos(theta);
                float z = ringRadius * std::sin(theta);

                Vec3 position(x, y, z);
                Vec3 normal = position.normalized();  // For sphere, normal = normalized position!

                mesh.vertices.push_back(Vertex(position, normal, color));
            }
        }

        // Generate indices
        for (int ring = 0; ring < rings; ring++) {
            for (int seg = 0; seg < segments; seg++) {
                uint32_t current = ring * (segments + 1) + seg;
                uint32_t next = current + segments + 1;

                // Two triangles per quad
                mesh.indices.push_back(current);
                mesh.indices.push_back(next);
                mesh.indices.push_back(current + 1);

                mesh.indices.push_back(current + 1);
                mesh.indices.push_back(next);
                mesh.indices.push_back(next + 1);
            }
        }

        return mesh;
    }

    // ==========================================================================
    // CREATE CORNER CUBE (3 perpendicular planes meeting at a corner)
    // Like the corner of a room: floor + back wall + left wall
    // Double-sided for better shadow visibility
    // Parameters:
    // - size: size of each plane
    // - color: color for all planes
    // ==========================================================================
    static Mesh createCornerCube(float size = 10.0f, const Color& color = Color::WHITE) {
        Mesh mesh;
        float half = size * 0.5f;

        // FLOOR PLANE (XZ plane at y=0) - DOUBLE SIDED
        // Top side (normal pointing up)
        Vec3 floorNormalUp(0, -1, 0);
        mesh.vertices.push_back(Vertex(Vec3(-half, 0, -half), floorNormalUp, color));  // 0
        mesh.vertices.push_back(Vertex(Vec3(half, 0, -half), floorNormalUp, color));   // 1
        mesh.vertices.push_back(Vertex(Vec3(half, 0, half), floorNormalUp, color));    // 2
        mesh.vertices.push_back(Vertex(Vec3(-half, 0, half), floorNormalUp, color));   // 3

        // Bottom side (normal pointing down)
        Vec3 floorNormalDown(0, 1, 0);
        mesh.vertices.push_back(Vertex(Vec3(-half, 0, -half), floorNormalDown, color));  // 4
        mesh.vertices.push_back(Vertex(Vec3(half, 0, -half), floorNormalDown, color));   // 5
        mesh.vertices.push_back(Vertex(Vec3(half, 0, half), floorNormalDown, color));    // 6
        mesh.vertices.push_back(Vertex(Vec3(-half, 0, half), floorNormalDown, color));   // 7

        // BACK WALL (XY plane at z=0) - DOUBLE SIDED
        // Front side (normal pointing forward)
        Vec3 backNormalFront(0, 0, 1);
        mesh.vertices.push_back(Vertex(Vec3(-half, 0, 0), backNormalFront, color));       // 8
        mesh.vertices.push_back(Vertex(Vec3(half, 0, 0), backNormalFront, color));        // 9
        mesh.vertices.push_back(Vertex(Vec3(half, size, 0), backNormalFront, color));     // 10
        mesh.vertices.push_back(Vertex(Vec3(-half, size, 0), backNormalFront, color));    // 11

        // Back side (normal pointing backward)
        Vec3 backNormalBack(0, 0, -1);
        mesh.vertices.push_back(Vertex(Vec3(-half, 0, 0), backNormalBack, color));        // 12
        mesh.vertices.push_back(Vertex(Vec3(half, 0, 0), backNormalBack, color));         // 13
        mesh.vertices.push_back(Vertex(Vec3(half, size, 0), backNormalBack, color));      // 14
        mesh.vertices.push_back(Vertex(Vec3(-half, size, 0), backNormalBack, color));     // 15

        // LEFT WALL (YZ plane at x=0) - DOUBLE SIDED
        // Right side (normal pointing right)
        Vec3 leftNormalRight(-1, 0, 0);
        mesh.vertices.push_back(Vertex(Vec3(0, 0, -half), leftNormalRight, color));       // 16
        mesh.vertices.push_back(Vertex(Vec3(0, 0, half), leftNormalRight, color));        // 17
        mesh.vertices.push_back(Vertex(Vec3(0, size, half), leftNormalRight, color));     // 18
        mesh.vertices.push_back(Vertex(Vec3(0, size, -half), leftNormalRight, color));    // 19

        // Left side (normal pointing left)
        Vec3 leftNormalLeft(1, 0, 0);
        mesh.vertices.push_back(Vertex(Vec3(0, 0, -half), leftNormalLeft, color));        // 20
        mesh.vertices.push_back(Vertex(Vec3(0, 0, half), leftNormalLeft, color));         // 21
        mesh.vertices.push_back(Vertex(Vec3(0, size, half), leftNormalLeft, color));      // 22
        mesh.vertices.push_back(Vertex(Vec3(0, size, -half), leftNormalLeft, color));     // 23

        // Indices for floor top (2 triangles, CCW winding)
        mesh.indices.push_back(0); mesh.indices.push_back(1); mesh.indices.push_back(2);
        mesh.indices.push_back(0); mesh.indices.push_back(2); mesh.indices.push_back(3);

        // Indices for floor bottom (2 triangles, reversed winding)
        mesh.indices.push_back(4); mesh.indices.push_back(6); mesh.indices.push_back(5);
        mesh.indices.push_back(4); mesh.indices.push_back(7); mesh.indices.push_back(6);

        // Indices for back wall front (2 triangles, CCW winding)
        mesh.indices.push_back(8); mesh.indices.push_back(9); mesh.indices.push_back(10);
        mesh.indices.push_back(8); mesh.indices.push_back(10); mesh.indices.push_back(11);

        // Indices for back wall back (2 triangles, reversed winding)
        mesh.indices.push_back(12); mesh.indices.push_back(14); mesh.indices.push_back(13);
        mesh.indices.push_back(12); mesh.indices.push_back(15); mesh.indices.push_back(14);

        // Indices for left wall right (2 triangles, CCW winding)
        mesh.indices.push_back(16); mesh.indices.push_back(17); mesh.indices.push_back(18);
        mesh.indices.push_back(16); mesh.indices.push_back(18); mesh.indices.push_back(19);

        // Indices for left wall left (2 triangles, reversed winding)
        mesh.indices.push_back(20); mesh.indices.push_back(22); mesh.indices.push_back(21);
        mesh.indices.push_back(20); mesh.indices.push_back(23); mesh.indices.push_back(22);

        return mesh;
    }

    // ==========================================================================
    // CREATE LETTER N (3D letter made of rectangular bars)
    // Composed of 3 parts:
    // - Left vertical bar
    // - Right vertical bar
    // - Diagonal bar connecting bottom-left to top-right
    // Parameters:
    // - height: height of the letter
    // - width: width of the letter
    // - thickness: thickness of the bars
    // - color: color of the letter
    // ==========================================================================
    static Mesh createLetterN(float height = 2.0f, float width = 1.5f, float thickness = 0.2f,
                              const Color& color = Color::WHITE) {
        Mesh mesh;

        // Helper function to add a box (rectangular prism)
        auto addBox = [&](Vec3 center, Vec3 extents, const Color& col) {
            uint32_t baseIdx = mesh.vertices.size();
            float hx = extents.x * 0.5f;
            float hy = extents.y * 0.5f;
            float hz = extents.z * 0.5f;

            // 8 corners relative to center
            Vec3 corners[8] = {
                Vec3(center.x - hx, center.y - hy, center.z - hz),  // 0
                Vec3(center.x + hx, center.y - hy, center.z - hz),  // 1
                Vec3(center.x - hx, center.y + hy, center.z - hz),  // 2
                Vec3(center.x + hx, center.y + hy, center.z - hz),  // 3
                Vec3(center.x - hx, center.y - hy, center.z + hz),  // 4
                Vec3(center.x + hx, center.y - hy, center.z + hz),  // 5
                Vec3(center.x - hx, center.y + hy, center.z + hz),  // 6
                Vec3(center.x + hx, center.y + hy, center.z + hz)   // 7
            };

            // Front face (z-)
            Vec3 frontNormal(0, 0, -1);
            mesh.vertices.push_back(Vertex(corners[0], frontNormal, col));
            mesh.vertices.push_back(Vertex(corners[1], frontNormal, col));
            mesh.vertices.push_back(Vertex(corners[3], frontNormal, col));
            mesh.vertices.push_back(Vertex(corners[2], frontNormal, col));

            // Back face (z+)
            Vec3 backNormal(0, 0, 1);
            mesh.vertices.push_back(Vertex(corners[5], backNormal, col));
            mesh.vertices.push_back(Vertex(corners[4], backNormal, col));
            mesh.vertices.push_back(Vertex(corners[6], backNormal, col));
            mesh.vertices.push_back(Vertex(corners[7], backNormal, col));

            // Left face (x-)
            Vec3 leftNormal(-1, 0, 0);
            mesh.vertices.push_back(Vertex(corners[4], leftNormal, col));
            mesh.vertices.push_back(Vertex(corners[0], leftNormal, col));
            mesh.vertices.push_back(Vertex(corners[2], leftNormal, col));
            mesh.vertices.push_back(Vertex(corners[6], leftNormal, col));

            // Right face (x+)
            Vec3 rightNormal(1, 0, 0);
            mesh.vertices.push_back(Vertex(corners[1], rightNormal, col));
            mesh.vertices.push_back(Vertex(corners[5], rightNormal, col));
            mesh.vertices.push_back(Vertex(corners[7], rightNormal, col));
            mesh.vertices.push_back(Vertex(corners[3], rightNormal, col));

            // Bottom face (y-)
            Vec3 bottomNormal(0, -1, 0);
            mesh.vertices.push_back(Vertex(corners[4], bottomNormal, col));
            mesh.vertices.push_back(Vertex(corners[5], bottomNormal, col));
            mesh.vertices.push_back(Vertex(corners[1], bottomNormal, col));
            mesh.vertices.push_back(Vertex(corners[0], bottomNormal, col));

            // Top face (y+)
            Vec3 topNormal(0, 1, 0);
            mesh.vertices.push_back(Vertex(corners[2], topNormal, col));
            mesh.vertices.push_back(Vertex(corners[3], topNormal, col));
            mesh.vertices.push_back(Vertex(corners[7], topNormal, col));
            mesh.vertices.push_back(Vertex(corners[6], topNormal, col));

            // Indices (2 triangles per face)
            for (uint32_t i = 0; i < 6; i++) {
                uint32_t base = baseIdx + i * 4;
                mesh.indices.push_back(base + 0);
                mesh.indices.push_back(base + 1);
                mesh.indices.push_back(base + 2);
                mesh.indices.push_back(base + 0);
                mesh.indices.push_back(base + 2);
                mesh.indices.push_back(base + 3);
            }
        };

        // Left vertical bar
        Vec3 leftCenter(-width * 0.5f + thickness * 0.5f, 0, 0);
        Vec3 leftExtents(thickness, height, thickness);
        addBox(leftCenter, leftExtents, color);

        // Right vertical bar
        Vec3 rightCenter(width * 0.5f - thickness * 0.5f, 0, 0);
        Vec3 rightExtents(thickness, height, thickness);
        addBox(rightCenter, rightExtents, color);

        // Diagonal bar (rotated box connecting bottom-left to top-right)
        float diagonalLength = std::sqrt(width * width + height * height);
        float angle = std::atan2(height, width);

        // Center of diagonal
        Vec3 diagCenter(0, 0, 0);
        // Create a rotated box for the diagonal
        // We'll add it as a simple box at angle
        Vec3 diagStart(-width * 0.5f + thickness * 0.5f, -height * 0.5f, 0);
        Vec3 diagEnd(width * 0.5f - thickness * 0.5f, height * 0.5f, 0);
        Vec3 diagMid = (diagStart + diagEnd) * 0.5f;

        // For simplicity, create a box aligned with the diagonal
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        // Simplified: add a stretched box along diagonal
        uint32_t baseIdx = mesh.vertices.size();

        // Create vertices for diagonal bar manually with proper orientation
        float halfThick = thickness * 0.5f;
        Vec3 perpX(-sinA, cosA, 0);  // perpendicular to diagonal in XY plane
        Vec3 perpZ(0, 0, 1);

        for (float t = 0.0f; t <= 1.0f; t += 0.5f) {
            Vec3 pos = diagStart * (1.0f - t) + diagEnd * t;
            // Add vertices around this position
            for (int i = 0; i < 4; i++) {
                float xOff = (i & 1) ? halfThick : -halfThick;
                float zOff = (i & 2) ? halfThick : -halfThick;
                Vec3 v = pos + perpX * xOff + perpZ * zOff;
                // Normal calculation would be complex, use averaged normal for simplicity
                Vec3 normal = (perpX * xOff + perpZ * zOff).normalized();
                mesh.vertices.push_back(Vertex(v, normal, color));
            }
        }

        // Simplified indices for diagonal (connecting 3 segments)
        uint32_t dBase = baseIdx;
        // Connect first ring to second ring
        for (int i = 0; i < 4; i++) {
            int next = (i + 1) % 4;
            mesh.indices.push_back(dBase + i);
            mesh.indices.push_back(dBase + next);
            mesh.indices.push_back(dBase + 4 + next);

            mesh.indices.push_back(dBase + i);
            mesh.indices.push_back(dBase + 4 + next);
            mesh.indices.push_back(dBase + 4 + i);
        }
        // Second to third ring
        for (int i = 0; i < 4; i++) {
            int next = (i + 1) % 4;
            mesh.indices.push_back(dBase + 4 + i);
            mesh.indices.push_back(dBase + 4 + next);
            mesh.indices.push_back(dBase + 8 + next);

            mesh.indices.push_back(dBase + 4 + i);
            mesh.indices.push_back(dBase + 8 + next);
            mesh.indices.push_back(dBase + 8 + i);
        }

        return mesh;
    }
};
