# 3D Renderer

3D renderer built in C++ with OpenGL 3.3 Core. GPU-accelerated rendering with conventional shadow mapping, full MVP transformation pipeline, and FPS camera controls.

Majorly WIP still. This is more of a learning experience than a tech showcase to me.
Also, ig not technically a renderer/rendering engine yet. still using OpenGL for that part. While the eventual goal is to have a standalone "engine" that can do that, for now im learning the basics of what would be expected out of a piece of software like that.

## Core Functionality

### **Mathematics** (`Vec2.h`, `Vec3.h`, `Vec4.h`, `Mat4.h`)
- Vector operations (dot, cross, normalize)
- 4×4 matrix transformations (translate, rotate, scale)
- LookAt and perspective projection matrices

### **Camera System** (`Camera.h`)
- First-person controls with yaw/pitch tracking
- View and projection matrix generation
- 6-DOF movement (WASD/QE/arrows)

### **Mesh Generation** (`Mesh.h`)
- Procedural cube, sphere, pyramid generators
- Indexed vertex buffers with normals and colors

### **OpenGL Rendering** (`RendererGL.h`, `Shaders.h`)
- GLSL vertex/fragment shaders for lighting
- Shadow mapping with two-pass rendering
- Framebuffer objects for depth texture
- VAO/VBO/IBO mesh uploading

### **Shadow Mapping** (`RendererGL.h:189-260`, `Shaders.h:156-207`)
- Orthographic light-space projection
- 2048×2048 depth texture
- Dynamic shadow bias calculation
- Pass 1: Render from light POV → depth map
- Pass 2: Render from camera POV → sample shadow map

### **Windowing** (`WindowGL.h`)
- SDL2 window creation and OpenGL context
- GLEW initialization for modern OpenGL
- Depth testing and backface culling setup

## License

MIT License
