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
// For a cube: 24 vertices vs 8 vertices + 36 indices = 67% memory reduction
//
// WINDING ORDER (CRITICAL):
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

    // Duplicate geometry with flipped normals so both sides render with correct lighting
    void makeDoubleSided() {
        size_t originalVertexCount = vertices.size();
        size_t originalIndexCount = indices.size();

        vertices.reserve(originalVertexCount * 2);
        indices.reserve(originalIndexCount * 2);

        for (size_t i = 0; i < originalVertexCount; ++i) {
            Vertex mirrored = vertices[i];
            mirrored.normal = -mirrored.normal;
            vertices.push_back(mirrored);
        }

        uint32_t offset = static_cast<uint32_t>(originalVertexCount);

        for (size_t i = 0; i < originalIndexCount; i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            indices.push_back(i0 + offset);
            indices.push_back(i2 + offset);
            indices.push_back(i1 + offset);
        }
    }

    // ==========================================================================
    // PRIMITIVE GENERATORS
    // These create basic 3D shapes - building blocks of 3D graphics
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
                Vec3 normal = position.normalized();  // For sphere, normal = normalized position

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
};
