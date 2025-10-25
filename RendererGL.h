#pragma once
#include <GL/glew.h>  // Must be before gl.h
#include <GL/gl.h>
#include "Mesh.h"
#include "Camera.h"
#include "Mat4.h"
#include "Shaders.h"
#include <iostream>
#include <vector>
#include <unordered_map>

// =============================================================================
// RendererGL: OpenGL GPU-accelerated 3D renderer
// =============================================================================
// ARCHITECTURE:
// - Compiles shaders (GPU programs)
// - Uploads meshes to GPU memory (VRAM)
// - Sends matrices and draws with minimal CPU work
//
// COMPARED TO Renderer3D (software):
// - No pixel loops!
// - No manual depth testing!
// - No framebuffer writes!
// - GPU does it all in hardware!
// =============================================================================

class RendererGL {
private:
    // ==========================================================================
    // SHADER PROGRAM
    // A shader program combines vertex + fragment shaders
    // Once compiled, it stays on GPU
    // ==========================================================================
    GLuint shaderProgram;

    // Uniform locations (where to send data to shaders)
    GLint uModelLoc;
    GLint uViewLoc;
    GLint uProjectionLoc;
    GLint uLightDirLoc;
    GLint uAmbientLoc;
    GLint uEmissiveLoc;
    GLint uLightSpaceMatrixLoc;  // For main shader
    GLint uShadowMapLoc;         // Shadow map texture sampler

    // ==========================================================================
    // SHADOW MAPPING
    // ==========================================================================
    GLuint shadowShaderProgram;    // Separate shader for shadow pass
    GLint uShadowModelLoc;         // Model matrix for shadow shader
    GLint uShadowLightSpaceLoc;    // Light space matrix for shadow shader

    GLuint shadowMapFBO;           // Framebuffer for shadow map rendering
    GLuint shadowMapTexture;       // Depth texture (the actual shadow map)

    static constexpr int SHADOW_MAP_WIDTH = 2048;   // Shadow map resolution
    static constexpr int SHADOW_MAP_HEIGHT = 2048;  // Higher = sharper shadows

    // ==========================================================================
    // MESH STORAGE
    // Each mesh uploaded to GPU gets an ID
    // We cache these to avoid re-uploading
    // ==========================================================================
    struct GPUMesh {
        GLuint vao;          // Vertex Array Object (state container)
        GLuint vbo;          // Vertex Buffer Object (vertex data)
        GLuint ibo;          // Index Buffer Object (triangle indices)
        GLsizei indexCount;  // Number of indices to draw
    };

    std::unordered_map<const Mesh*, GPUMesh> uploadedMeshes;

public:
    RendererGL() {
        compileShaders();
        compileShadowShaders();
        setupUniforms();
        setupShadowMapping();
    }

    ~RendererGL() {
        // Clean up uploaded meshes
        for (auto& [mesh, gpuMesh] : uploadedMeshes) {
            glDeleteVertexArrays(1, &gpuMesh.vao);
            glDeleteBuffers(1, &gpuMesh.vbo);
            glDeleteBuffers(1, &gpuMesh.ibo);
        }

        // Clean up shader programs
        glDeleteProgram(shaderProgram);
        glDeleteProgram(shadowShaderProgram);

        // Clean up shadow mapping resources
        glDeleteTextures(1, &shadowMapTexture);
        glDeleteFramebuffers(1, &shadowMapFBO);
    }

    // ==========================================================================
    // DRAW MESH
    // This is SO MUCH SIMPLER than software renderer!
    //
    // Compare to Renderer3D::drawMesh (Renderer3D.h:31-183)
    // - No vertex loops
    // - No pixel loops
    // - No manual depth testing
    // - Just set uniforms and call draw!
    // ==========================================================================
    void drawMesh(const Mesh& mesh,
                  const Mat4& modelMatrix,
                  Camera& camera,
                  const Mat4& lightSpaceMatrix,
                  bool emissive = false) {

        // ======================================================================
        // ENSURE MESH IS ON GPU
        // Upload if this is first time seeing this mesh
        // ======================================================================
        if (uploadedMeshes.find(&mesh) == uploadedMeshes.end()) {
            uploadMesh(mesh);
        }

        const GPUMesh& gpuMesh = uploadedMeshes[&mesh];

        // ======================================================================
        // ACTIVATE SHADER PROGRAM
        // All following draw calls use this shader
        // ======================================================================
        glUseProgram(shaderProgram);

        // ======================================================================
        // SEND UNIFORMS TO GPU
        // These are "global variables" accessible in shaders
        // ======================================================================

        // Matrices (CPU → GPU, 16 floats each)
        glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, modelMatrix.m);
        glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, camera.getViewMatrix().m);
        glUniformMatrix4fv(uProjectionLoc, 1, GL_FALSE, camera.getProjectionMatrix().m);
        glUniformMatrix4fv(uLightSpaceMatrixLoc, 1, GL_FALSE, lightSpaceMatrix.m);

        // Light direction (CPU → GPU, 3 floats)
        Vec3 lightDir = Vec3(0.3f, 0.8f, 0.5f).normalized();
        glUniform3f(uLightDirLoc, lightDir.x, lightDir.y, lightDir.z);

        // Ambient lighting (CPU → GPU, 1 float)
        glUniform1f(uAmbientLoc, 0.3f);

        // Emissive flag (CPU → GPU, bool)
        // If true, object emits light (self-illuminated, not affected by lighting)
        glUniform1i(uEmissiveLoc, emissive ? GL_TRUE : GL_FALSE);

        // Bind shadow map texture to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
        glUniform1i(uShadowMapLoc, 0);  // Tell shader to use texture unit 0

        // ======================================================================
        // BIND VERTEX ARRAY
        // This sets up all vertex attribute pointers
        // (position, normal, color → shader inputs)
        // ======================================================================
        glBindVertexArray(gpuMesh.vao);

        // ======================================================================
        // DRAW CALL
        // This is where the magic happens!
        // GPU processes ALL vertices and pixels in PARALLEL
        //
        // What happens on GPU:
        // 1. Vertex shader runs on each vertex (in parallel)
        // 2. GPU rasterizes triangles (hardware)
        // 3. Fragment shader runs on each pixel (in parallel)
        // 4. GPU depth tests (hardware)
        // 5. GPU writes to framebuffer (hardware)
        //
        // All in microseconds!
        // ======================================================================
        glDrawElements(
            GL_TRIANGLES,              // Draw triangles
            gpuMesh.indexCount,        // Number of indices
            GL_UNSIGNED_INT,           // Index type
            nullptr                    // Offset in IBO (0 = start)
        );

        // Unbind (good practice, prevents accidental modifications)
        glBindVertexArray(0);
    }

    // ==========================================================================
    // SHADOW PASS: BEGIN
    // Sets up for rendering from light's perspective (depth only)
    // ==========================================================================
    void beginShadowPass() {
        // Bind shadow map framebuffer (render to shadow map texture)
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

        // Set viewport to shadow map resolution
        glViewport(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

        // Clear only depth buffer (we don't have color attachment)
        glClear(GL_DEPTH_BUFFER_BIT);

        // Optional: Enable front-face culling for shadow pass
        // This helps reduce "shadow acne" (render back faces only)
        // Uncomment if you get self-shadowing artifacts:
        // glCullFace(GL_FRONT);
    }

    // ==========================================================================
    // SHADOW PASS: RENDER MESH
    // Render mesh to shadow map (depth only, from light's POV)
    // ==========================================================================
    void renderShadowMesh(const Mesh& mesh,
                          const Mat4& modelMatrix,
                          const Mat4& lightSpaceMatrix) {
        // ======================================================================
        // ENSURE MESH IS ON GPU
        // ======================================================================
        if (uploadedMeshes.find(&mesh) == uploadedMeshes.end()) {
            uploadMesh(mesh);
        }

        const GPUMesh& gpuMesh = uploadedMeshes[&mesh];

        // ======================================================================
        // USE SHADOW SHADER
        // Simple shader that only writes depth
        // ======================================================================
        glUseProgram(shadowShaderProgram);

        // ======================================================================
        // SEND UNIFORMS
        // Only need model matrix and light space matrix
        // ======================================================================
        glUniformMatrix4fv(uShadowModelLoc, 1, GL_FALSE, modelMatrix.m);
        glUniformMatrix4fv(uShadowLightSpaceLoc, 1, GL_FALSE, lightSpaceMatrix.m);

        // ======================================================================
        // DRAW
        // GPU writes depth values to shadow map texture
        // ======================================================================
        glBindVertexArray(gpuMesh.vao);
        glDrawElements(GL_TRIANGLES, gpuMesh.indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    // ==========================================================================
    // SHADOW PASS: END
    // Restore normal rendering state
    // ==========================================================================
    void endShadowPass(int screenWidth, int screenHeight) {
        // Restore default framebuffer (render to screen)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Restore viewport to screen size
        glViewport(0, 0, screenWidth, screenHeight);

        // Restore back-face culling if we changed it
        // glCullFace(GL_BACK);
    }

private:
    // ==========================================================================
    // COMPILE SHADERS
    // Turns GLSL source code into GPU executable
    // ==========================================================================
    void compileShaders() {
        // ======================================================================
        // VERTEX SHADER
        // ======================================================================
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &Shaders::VERTEX_SHADER, nullptr);
        glCompileShader(vertexShader);

        // Check for compilation errors
        checkShaderCompilation(vertexShader, "VERTEX");

        // ======================================================================
        // FRAGMENT SHADER
        // ======================================================================
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &Shaders::FRAGMENT_SHADER, nullptr);
        glCompileShader(fragmentShader);

        // Check for compilation errors
        checkShaderCompilation(fragmentShader, "FRAGMENT");

        // ======================================================================
        // LINK SHADER PROGRAM
        // Combines vertex + fragment shaders into complete program
        // ======================================================================
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // Check for linking errors
        checkProgramLinking(shaderProgram);

        // Clean up (shaders are now part of program, don't need shader objects)
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        std::cout << "Shaders compiled and linked successfully!" << std::endl;
    }

    // ==========================================================================
    // COMPILE SHADOW SHADERS
    // Simpler shaders for depth-only shadow pass
    // ==========================================================================
    void compileShadowShaders() {
        // ======================================================================
        // SHADOW VERTEX SHADER
        // ======================================================================
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &Shaders::SHADOW_VERTEX_SHADER, nullptr);
        glCompileShader(vertexShader);
        checkShaderCompilation(vertexShader, "SHADOW_VERTEX");

        // ======================================================================
        // SHADOW FRAGMENT SHADER
        // ======================================================================
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &Shaders::SHADOW_FRAGMENT_SHADER, nullptr);
        glCompileShader(fragmentShader);
        checkShaderCompilation(fragmentShader, "SHADOW_FRAGMENT");

        // ======================================================================
        // LINK SHADOW SHADER PROGRAM
        // ======================================================================
        shadowShaderProgram = glCreateProgram();
        glAttachShader(shadowShaderProgram, vertexShader);
        glAttachShader(shadowShaderProgram, fragmentShader);
        glLinkProgram(shadowShaderProgram);
        checkProgramLinking(shadowShaderProgram);

        // Clean up
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        std::cout << "Shadow shaders compiled and linked successfully!" << std::endl;
    }

    // ==========================================================================
    // SETUP SHADOW MAPPING
    // Creates framebuffer and depth texture for shadow map
    // ==========================================================================
    void setupShadowMapping() {
        // ======================================================================
        // CREATE DEPTH TEXTURE (the shadow map itself)
        // ======================================================================
        glGenTextures(1, &shadowMapTexture);
        glBindTexture(GL_TEXTURE_2D, shadowMapTexture);

        // Allocate texture storage (depth only, no color)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                     SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        // Texture filtering (nearest = hard shadows, linear = softer)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Clamp to border (texels outside shadow map = no shadow)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};  // White = lit (no shadow)
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // ======================================================================
        // CREATE FRAMEBUFFER FOR SHADOW RENDERING
        // ======================================================================
        glGenFramebuffers(1, &shadowMapFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

        // Attach depth texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, shadowMapTexture, 0);

        // We don't need color output for shadow pass
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        // Check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR: Shadow map framebuffer is not complete!" << std::endl;
        }

        // Unbind framebuffer (return to default)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        std::cout << "Shadow map created: " << SHADOW_MAP_WIDTH << "x" << SHADOW_MAP_HEIGHT << std::endl;
    }

    // ==========================================================================
    // GET UNIFORM LOCATIONS
    // Find where to send data to shaders
    // ==========================================================================
    void setupUniforms() {
        // Main shader uniforms
        uModelLoc = glGetUniformLocation(shaderProgram, "uModel");
        uViewLoc = glGetUniformLocation(shaderProgram, "uView");
        uProjectionLoc = glGetUniformLocation(shaderProgram, "uProjection");
        uLightDirLoc = glGetUniformLocation(shaderProgram, "uLightDir");
        uAmbientLoc = glGetUniformLocation(shaderProgram, "uAmbient");
        uEmissiveLoc = glGetUniformLocation(shaderProgram, "uEmissive");
        uLightSpaceMatrixLoc = glGetUniformLocation(shaderProgram, "uLightSpaceMatrix");
        uShadowMapLoc = glGetUniformLocation(shaderProgram, "uShadowMap");

        // Shadow shader uniforms
        uShadowModelLoc = glGetUniformLocation(shadowShaderProgram, "uModel");
        uShadowLightSpaceLoc = glGetUniformLocation(shadowShaderProgram, "uLightSpaceMatrix");
    }

    // ==========================================================================
    // UPLOAD MESH TO GPU
    // This happens ONCE per mesh (then stays in VRAM)
    // ==========================================================================
    void uploadMesh(const Mesh& mesh) {
        GPUMesh gpuMesh;

        // ======================================================================
        // CREATE VERTEX ARRAY OBJECT (VAO)
        // Stores vertex attribute configuration
        // Think of it as a "state container" for vertex setup
        // ======================================================================
        glGenVertexArrays(1, &gpuMesh.vao);
        glBindVertexArray(gpuMesh.vao);

        // ======================================================================
        // CREATE AND FILL VERTEX BUFFER (VBO)
        // Upload vertex data to GPU memory (VRAM)
        // ======================================================================
        glGenBuffers(1, &gpuMesh.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.vbo);

        // Upload data: CPU RAM → GPU VRAM
        // GL_STATIC_DRAW: data won't change (GPU can optimize)
        glBufferData(GL_ARRAY_BUFFER,
                     mesh.vertices.size() * sizeof(Vertex),
                     mesh.vertices.data(),
                     GL_STATIC_DRAW);

        // ======================================================================
        // CONFIGURE VERTEX ATTRIBUTES
        // Tell GPU how to interpret vertex data
        // ======================================================================

        // Position (location = 0 in vertex shader)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,                  // Attribute location
            3,                  // Number of components (x, y, z)
            GL_FLOAT,           // Data type
            GL_FALSE,           // Don't normalize
            sizeof(Vertex),     // Stride (bytes between vertices)
            (void*)offsetof(Vertex, position)  // Offset to position in struct
        );

        // Normal (location = 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void*)offsetof(Vertex, normal)
        );

        // Color (location = 2)
        // Colors are uint8_t but shader expects vec3 (floats 0-1)
        // GL_TRUE normalizes: 0-255 → 0.0-1.0
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(
            2,
            3,                  // R, G, B (ignore alpha for now)
            GL_UNSIGNED_BYTE,   // uint8_t
            GL_TRUE,            // Normalize to 0-1
            sizeof(Vertex),
            (void*)offsetof(Vertex, color)
        );

        // ======================================================================
        // CREATE AND FILL INDEX BUFFER (IBO / EBO)
        // Upload triangle indices to GPU
        // ======================================================================
        glGenBuffers(1, &gpuMesh.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.ibo);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     mesh.indices.size() * sizeof(uint32_t),
                     mesh.indices.data(),
                     GL_STATIC_DRAW);

        gpuMesh.indexCount = static_cast<GLsizei>(mesh.indices.size());

        // Unbind
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Store for later use
        uploadedMeshes[&mesh] = gpuMesh;

        std::cout << "Uploaded mesh: " << mesh.vertices.size() << " vertices, "
                  << mesh.getTriangleCount() << " triangles" << std::endl;
    }

    // ==========================================================================
    // ERROR CHECKING
    // ==========================================================================
    void checkShaderCompilation(GLuint shader, const char* type) {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "ERROR: " << type << " shader compilation failed:\n"
                      << infoLog << std::endl;
        }
    }

    void checkProgramLinking(GLuint program) {
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "ERROR: Shader program linking failed:\n"
                      << infoLog << std::endl;
        }
    }
};
