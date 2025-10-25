#pragma once
#include <string>

// =============================================================================
// GLSL Shaders: Programs that run on the GPU
// =============================================================================
// WHAT ARE SHADERS?
// - Small programs written in GLSL (OpenGL Shading Language)
// - Compiled and run ON THE GPU (not CPU)
// - Massively parallel (run on thousands of cores simultaneously)
//
// TWO TYPES WE USE:
// 1. VERTEX SHADER: Runs once per vertex (transforms 3D → 2D)
// 2. FRAGMENT SHADER: Runs once per pixel (determines color)
//
// THE PIPELINE:
// Vertices → VERTEX SHADER → Rasterization (GPU) → FRAGMENT SHADER → Pixels
//
// This is EXACTLY what our software renderer does, but on GPU hardware!
// =============================================================================

namespace Shaders {

// =============================================================================
// VERTEX SHADER
// =============================================================================
// RUNS ON: GPU, once per vertex
// INPUT: Vertex position, normal, color
// OUTPUT: Transformed position (clip space), data for fragment shader
//
// THIS REPLACES: All our CPU matrix math (Renderer3D.h:44-58)
//
// GLSL VERSION: 330 core = OpenGL 3.3 Core Profile
// =============================================================================

const char* VERTEX_SHADER = R"(
#version 330 core

// =============================================================================
// INPUT ATTRIBUTES (from CPU vertex buffer)
// layout(location = X) matches glVertexAttribPointer calls
// =============================================================================
layout(location = 0) in vec3 aPosition;  // Vertex position (object space)
layout(location = 1) in vec3 aNormal;    // Vertex normal (object space)
layout(location = 2) in vec3 aColor;     // Vertex color (RGB, 0-1 range)

// =============================================================================
// UNIFORMS (constant for all vertices in a draw call)
// These are set from CPU with glUniform* calls
// =============================================================================
uniform mat4 uModel;       // Model matrix (object → world)
uniform mat4 uView;        // View matrix (world → camera)
uniform mat4 uProjection;  // Projection matrix (camera → clip space)

// We could combine into MVP on CPU, but keeping separate for clarity

// =============================================================================
// OUTPUTS (passed to fragment shader)
// GPU automatically interpolates these across triangle
// =============================================================================
out vec3 fragColor;      // Color (interpolated per-pixel)
out vec3 fragNormal;     // Normal (interpolated per-pixel)
out vec3 fragWorldPos;   // World position (for shadow mapping)
out vec4 fragLightSpace; // Position in light's clip space (for shadow mapping)

// =============================================================================
// UNIFORMS for shadow mapping
// =============================================================================
uniform mat4 uLightSpaceMatrix;  // Light's view-projection matrix

// =============================================================================
// MAIN FUNCTION
// This runs on GPU for EVERY vertex in the mesh
// For a sphere with 800 vertices, this runs 800 times in PARALLEL
// =============================================================================
void main() {
    // ==========================================================================
    // POSITION TRANSFORMATION
    // This is EXACTLY our CPU code: MVP * vertex
    // (Renderer3D.h:51: Vec4 clipV0 = mvp * Vec4(v0.position, 1.0f))
    //
    // gl_Position is special: tells GPU where vertex is in clip space
    // ==========================================================================
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);

    // ==========================================================================
    // WORLD POSITION
    // Calculate world-space position for shadow mapping
    // ==========================================================================
    fragWorldPos = vec3(uModel * vec4(aPosition, 1.0));

    // ==========================================================================
    // LIGHT SPACE POSITION
    // Transform to light's clip space for shadow map lookup
    // ==========================================================================
    fragLightSpace = uLightSpaceMatrix * vec4(fragWorldPos, 1.0);

    // ==========================================================================
    // NORMAL TRANSFORMATION
    // Transform normal from object space to world space
    // This is what we do in Renderer3D.h:140:
    //   Vec3 worldNormal = modelMatrix.transformDirection(objectNormal)
    //
    // mat3(uModel) extracts rotation/scale, discards translation (correct for normals)
    // ==========================================================================
    fragNormal = normalize(mat3(uModel) * aNormal);

    // ==========================================================================
    // PASS COLOR TO FRAGMENT SHADER
    // GPU will automatically interpolate across triangle
    // (This is what barycentric coordinates do in our software renderer)
    // ==========================================================================
    fragColor = aColor;
}
)";

// =============================================================================
// FRAGMENT SHADER
// =============================================================================
// RUNS ON: GPU, once per pixel inside triangle
// INPUT: Interpolated data from vertex shader
// OUTPUT: Final pixel color
//
// THIS REPLACES: Our lighting calculation and pixel writing
//                (Renderer3D.h:136-170)
//
// =============================================================================

const char* FRAGMENT_SHADER = R"(
#version 330 core

// =============================================================================
// INPUTS (from vertex shader, interpolated by GPU)
// These vary per-pixel - GPU automatically interpolates using barycentric coords
// =============================================================================
in vec3 fragColor;       // Interpolated color
in vec3 fragNormal;      // Interpolated normal (NOT normalized after interpolation)
in vec3 fragWorldPos;    // Interpolated world position
in vec4 fragLightSpace;  // Interpolated light-space position

// =============================================================================
// UNIFORMS (constant for all pixels in a draw call)
// =============================================================================
uniform vec3 uLightDir;       // Light direction (world space)
uniform float uAmbient;       // Ambient light amount (0-1)
uniform bool uEmissive;       // If true, object emits light (unlit, self-illuminated)
uniform sampler2D uShadowMap; // Shadow map texture (depth from light's POV)

// =============================================================================
// OUTPUT
// This is the final pixel color that appears on screen
// =============================================================================
out vec4 finalColor;

// =============================================================================
// SHADOW CALCULATION FUNCTION
// Determines if this fragment is in shadow by comparing depth with shadow map
// =============================================================================
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // ==========================================================================
    // PERSPECTIVE DIVIDE
    // Convert from clip space [-w, w] to NDC [-1, 1]
    // ==========================================================================
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // ==========================================================================
    // TRANSFORM TO TEXTURE COORDINATES
    // Shadow map texture uses [0, 1] range, NDC is [-1, 1]
    // ==========================================================================
    projCoords = projCoords * 0.5 + 0.5;

    // ==========================================================================
    // CHECK IF OUTSIDE SHADOW MAP
    // If fragment is outside light's view frustum, it's not in shadow
    // ==========================================================================
    if (projCoords.z > 1.0) {
        return 0.0;  // Not in shadow (outside light's far plane)
    }

    // ==========================================================================
    // SAMPLE SHADOW MAP
    // Get the depth value stored from light's perspective
    // ==========================================================================
    float closestDepth = texture(uShadowMap, projCoords.xy).r;

    // ==========================================================================
    // CURRENT FRAGMENT DEPTH
    // How far is this fragment from the light?
    // ==========================================================================
    float currentDepth = projCoords.z;

    // ==========================================================================
    // SHADOW BIAS
    // Prevents "shadow acne" (self-shadowing artifacts)
    // Larger bias for surfaces perpendicular to light
    // ==========================================================================
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    // ==========================================================================
    // SHADOW TEST
    // If current depth > stored depth, fragment is behind something = shadow
    // ==========================================================================
    float shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;

    return shadow;
}

// =============================================================================
// MAIN FUNCTION
// This runs on GPU for EVERY pixel inside EVERY triangle
//
// For 830 triangles × 50×50 avg pixels = 2 MILLION times per frame
// =============================================================================
void main() {
    // ==========================================================================
    // EMISSIVE MATERIALS
    // If object is emissive (like a light source), skip lighting calculations
    // and just output the color at full brightness
    // ==========================================================================
    if (uEmissive) {
        finalColor = vec4(fragColor, 1.0);
        return;
    }

    // ==========================================================================
    // NORMALIZE INTERPOLATED NORMAL
    // After interpolation, normals aren't unit length anymore
    // (Same issue we'd have if we interpolated normals in software renderer)
    // ==========================================================================
    vec3 normal = normalize(fragNormal);
    float facing = dot(normal, uLightDir);
    vec3 litNormal = facing < 0.0 ? -normal : normal;

    // ==========================================================================
    // CALCULATE SHADOW
    // 0.0 = fully lit, 1.0 = fully shadowed
    // ==========================================================================
    float shadow = calculateShadow(fragLightSpace, litNormal, uLightDir);

    // ==========================================================================
    // LAMBERTIAN DIFFUSE LIGHTING
    // EXACTLY the same as Renderer3D.h:145-151:
    //   float diffuse = std::max(0.0f, worldNormal.dot(lightDir));
    //   float brightness = ambient + (1.0f - ambient) * diffuse;
    //
    // NOW WITH SHADOWS: multiply diffuse by (1.0 - shadow)
    // If shadow = 1.0, diffuse becomes 0 (only ambient light remains)
    // ==========================================================================
    float diffuse = max(abs(facing), 0.0);
    float brightness = uAmbient + (1.0 - uAmbient) * diffuse * (1.0 - shadow);

    // ==========================================================================
    // APPLY LIGHTING TO COLOR
    // Same as Renderer3D.h:165-170:
    //   Color litColor(baseColor.r * brightness, ...)
    // ==========================================================================
    vec3 litColor = fragColor * brightness;

    // ==========================================================================
    // OUTPUT FINAL COLOR
    // vec4 includes alpha channel (1.0 = fully opaque)
    // ==========================================================================
    finalColor = vec4(litColor, 1.0);
}
)";

// =============================================================================
// COMPARISON: CPU vs GPU
// =============================================================================
//
// CPU (Renderer3D.h):
// - Loop through each vertex: transform with Mat4 * Vec4
// - Loop through each pixel: calculate barycentric, interpolate, light, write
// - Single-threaded, millions of iterations
//
// GPU (These Shaders):
// - Vertex shader: runs on ALL vertices in PARALLEL
// - Fragment shader: runs on ALL pixels in PARALLEL
// - Thousands of cores, billions of operations/second
//
// RESULT: 100-1000× faster
// =============================================================================

// =============================================================================
// SHADOW MAPPING SHADERS
// =============================================================================
// These shaders are used for the shadow pass (rendering from light's POV)
// We only need to write depth, so they're much simpler than main shaders
// =============================================================================

const char* SHADOW_VERTEX_SHADER = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uLightSpaceMatrix;  // Combined light view-projection matrix
uniform mat4 uModel;             // Model matrix

void main() {
    // Transform vertex to light's clip space
    // This is exactly like camera rendering, but from light's perspective
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPosition, 1.0);
}
)";

const char* SHADOW_FRAGMENT_SHADER = R"(
#version 330 core

void main() {
    // We don't need to write anything
    // OpenGL automatically writes gl_FragDepth (the depth value)
    // That's all we need for the shadow map
}
)";

} // namespace Shaders
