# Practical Tools for Simple Design

###### Officially Supported Platforms and IDEs

|                                | Windows | macOS | Linux |
|:------------------------------:|:-------:|:-----:|:-----:|
|             CLion              |    V    |   V   |   V   |
|        VSCode[^codeoss]        |    V    |   V   |   V   |
| Visual Studio[^novs][^vsmacos] |    ?    |   X   |   X   |
|             No IDE             |    V    |   V   |   V   |

[^vsmacos]: [Microsoft Kills Visual Studio for Mac](https://visualstudiomagazine.com/articles/2023/08/30/vs-for-mac-retirement.aspx)
[^codeoss]: On Linux the support of Code - OSS and VSCodium aren't guaranteed.
[^novs]: Due to lack of testing there may or may not be more issues on VS. Anyway, building PTSD on VS is available.

## Getting Started

Required: Git, CMake, C/C++ Compiler, OpenGL Implementation

Optional: Ninja Build, Clang

> You might get some issue like https://github.com/ntut-open-source-club/practical-tools-for-simple-design/issues/78 check it if you need.

### Command Line

[//]: # (TODO: No IDE Quick Start)
> [!WARNING]  
> This section is work in progress.

```
git clone https://github.com/ntut-open-source-club/practical-tools-for-simple-design.git
cd practical-tools-for-simple-design
cmake -B build
cmake --build build
```

> If Ninja Build is install use `cmake -B build -G Ninja` to speed compile time

> For older versions of CMake(`<3.13`? verification needed) use
> ```
> mkdir build
> cd build
> cmake .
> cmake --build .
> ```
> if the `-B` flag is unsupported

> If using Neovim or other LSP supported editors, append `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to the generation command for `clangd` to work

### VSCode

[//]: # (TODO: VSCode Quick Start)
> [!WARNING]  
> This section is work in progress.

### CLion

[CLion Quick Start](.github/docs/CLionQuickStart/CLionQuickStart.md)

###### NOTE: If you have time, read [OOP2023f Environment Setup](https://hackmd.io/@OOP2023f/rk2-8cVCh)

## Generate Doxygen Documents

Required: Doxygen 1.9.6

```
doxygen docs/Doxyfile
```

Open the generated documents with your favorite browser at `docs/html/index.html`

---

## 3D Framework Extension

PTSD now includes a complete **3D rendering framework** that coexists with the original 2D system. You can build pure 3D apps, pure 2D apps, or hybrid 2D+3D apps.

### Architecture Overview

```
Namespaces:
  Core3D::   - Low-level 3D primitives (Camera, Mesh, Model, Cubemap, Framebuffer, Skeleton)
  Scene::    - Scene management (SceneGraph, SceneNode, Light, Skybox)
  Render::   - Rendering pipeline (ForwardRenderer, ShadowMap, PBRMaterial)
```

**Rendering Pipeline** (Forward Rendering):
1. **Shadow Pass** — Depth-only render from directional light POV
2. **Geometry Pass** — Render all scene nodes with Phong or PBR shading
3. **Skybox Pass** — Draw cubemap skybox at infinite distance

### Quick Start (3D)

```cpp
#include "Core/Context.hpp"
#include "Core3D/Camera.hpp"
#include "Core3D/Model.hpp"
#include "Render/ForwardRenderer.hpp"
#include "Scene/SceneGraph.hpp"
#include "Scene/SceneNode.hpp"

int main(int, char**) {
    auto context = Core::Context::GetInstance();

    // 1. Setup scene
    Scene::SceneGraph scene;
    Render::ForwardRenderer renderer;

    // 2. Camera
    scene.GetCamera().SetPosition(glm::vec3(0, 5, 10));

    // 3. Load a model
    auto model = std::make_shared<Core3D::Model>("assets/models/scene.obj");
    auto node = std::make_shared<Scene::SceneNode>();
    node->SetDrawable(model);
    scene.GetRoot()->AddChild(node);

    // 4. Add a directional light
    Scene::DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-1, -1, -1));
    sun.color = glm::vec3(1.0f);
    sun.intensity = 2.0f;
    scene.SetDirectionalLight(sun);

    // 5. Main loop
    while (!context->GetExit()) {
        context->Setup();
        renderer.Render(scene);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        context->Update();
    }
    return 0;
}
```

Build the 3D example:
```bash
cmake -B build
cmake --build build --target Example3D
```

### Camera

`Core3D::Camera` provides an FPS-style camera with perspective/orthographic projection.

```cpp
auto &camera = scene.GetCamera();
camera.SetPosition(glm::vec3(0, 5, 10));
camera.SetYaw(-90.0f);   // Look along -Z
camera.SetPitch(-15.0f);  // Slightly downward

// In update loop:
float dt = deltaTime;
camera.ProcessKeyboard(Core3D::CameraMovement::FORWARD, dt);  // W
camera.ProcessKeyboard(Core3D::CameraMovement::LEFT, dt);     // A
camera.ProcessMouseMovement(xOffset, yOffset);                 // Mouse look
camera.ProcessMouseScroll(scrollY);                            // Zoom (FOV)

// Matrices for rendering:
glm::mat4 view = camera.GetViewMatrix();
glm::mat4 proj = camera.GetProjectionMatrix();
```

### Model Loading

`Core3D::Model` uses Assimp to load 40+ 3D formats (OBJ, FBX, glTF, DAE, 3DS, etc.):

```cpp
// Load from file (auto-extracts textures from model directory)
auto backpack = std::make_shared<Core3D::Model>("assets/models/backpack/backpack.obj");

// Flip UVs if textures appear upside-down
auto model = std::make_shared<Core3D::Model>("model.fbx", true);

// Create procedural geometry
std::vector<Core3D::Vertex3D> verts = { ... };
std::vector<unsigned int> indices = { ... };
std::vector<Core3D::MeshTexture> textures = {};
std::vector<Core3D::Mesh> meshes;
meshes.emplace_back(verts, indices, textures);
auto proceduralModel = std::make_shared<Core3D::Model>(std::move(meshes));
```

`Vertex3D` layout:
| Field       | Type      | Description                  |
|:-----------:|:---------:|:----------------------------:|
| position    | vec3      | Vertex position              |
| normal      | vec3      | Surface normal               |
| texCoords   | vec2      | UV coordinates               |
| tangent     | vec3      | Tangent (for normal mapping)  |
| bitangent   | vec3      | Bitangent                    |
| boneIDs     | int[4]    | Skeleton bone indices         |
| boneWeights | float[4]  | Bone influence weights        |

### Lighting

Three light types are supported:

```cpp
// Directional light (sun)
Scene::DirectionalLight sun;
sun.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
sun.color = glm::vec3(1.0f, 0.98f, 0.92f);
sun.intensity = 1.5f;
scene.SetDirectionalLight(sun);

// Point light (lamp, torch, etc.)
Scene::PointLight lamp;
lamp.position = glm::vec3(3.0f, 4.0f, 2.0f);
lamp.color = glm::vec3(0.8f, 0.9f, 1.0f);
lamp.intensity = 8.0f;
lamp.constant = 1.0f;
lamp.linear = 0.09f;
lamp.quadratic = 0.032f;
scene.AddPointLight(lamp);

// Spot light (flashlight, stage light)
Scene::SpotLight spot;
spot.position = glm::vec3(0, 5, 0);
spot.direction = glm::vec3(0, -1, 0);
spot.color = glm::vec3(1.0f);
spot.intensity = 10.0f;
spot.cutOff = glm::cos(glm::radians(12.5f));
spot.outerCutOff = glm::cos(glm::radians(17.5f));
scene.AddSpotLight(spot);
```

Maximum lights: **16 point lights**, **8 spot lights** (configurable in `config.json`).

### Materials

#### Blinn-Phong Material
```cpp
auto material = std::make_shared<Core3D::Material>();
material->ambient = glm::vec3(0.1f);
material->diffuse = glm::vec3(0.8f, 0.2f, 0.2f);
material->specular = glm::vec3(1.0f);
material->shininess = 64.0f;
// Optional texture maps:
// material->diffuseMap = loadedTexture;
// material->normalMap = normalTexture;
node->SetMaterial(material);
```

#### PBR Material
```cpp
Render::PBRMaterial pbrMat;
pbrMat.albedo = glm::vec3(0.5f, 0.0f, 0.0f);
pbrMat.metallic = 0.9f;
pbrMat.roughness = 0.1f;
pbrMat.ao = 1.0f;
// Or use texture maps:
// pbrMat.albedoMap = albedoTex;
// pbrMat.normalMap = normalTex;
// pbrMat.metallicMap = metallicTex;
// pbrMat.roughnessMap = roughnessTex;
// pbrMat.aoMap = aoTex;
```

Toggle between Phong and PBR:
```cpp
renderer.SetPBREnabled(true);   // PBR (Cook-Torrance BRDF)
renderer.SetPBREnabled(false);  // Blinn-Phong
```

### Shadow Mapping

Shadows are enabled by default for the directional light. Uses PCF (Percentage Closer Filtering) for soft edges.

```cpp
renderer.SetShadowsEnabled(true);    // enable
renderer.SetShadowsEnabled(false);   // disable
renderer.SetSceneRadius(50.0f);      // adjust shadow frustum size

// Shadow map resolution in config.json:
// "shadow_map_size": 2048
```

### Skybox

```cpp
auto skybox = std::make_shared<Scene::Skybox>(std::vector<std::string>{
    "assets/skybox/right.jpg",
    "assets/skybox/left.jpg",
    "assets/skybox/top.jpg",
    "assets/skybox/bottom.jpg",
    "assets/skybox/front.jpg",
    "assets/skybox/back.jpg"
});
scene.SetSkybox(skybox);
```

### Skeletal Animation

```cpp
#include "Core3D/Skeleton.hpp"
#include "Core3D/BoneAnimation.hpp"

// Load an animated model
auto animModel = std::make_shared<Core3D::Model>("assets/models/character.fbx");

// Load animation from the same or different file
Core3D::BoneAnimation animation("assets/models/character.fbx");

// In update loop:
float currentTime = elapsedSeconds;
auto boneMatrices = animation.UpdateBoneTransforms(currentTime);

// Upload bone matrices to the Skeletal.vert shader (uniform mat4 boneMatrices[128])
```

### Scene Graph

Nodes form a parent-child tree. Child transforms are relative to their parent:

```cpp
auto root = scene.GetRoot();

auto parent = std::make_shared<Scene::SceneNode>();
parent->SetPosition(glm::vec3(0, 0, -5));
root->AddChild(parent);

auto child = std::make_shared<Scene::SceneNode>();
child->SetPosition(glm::vec3(2, 0, 0));  // offset from parent
child->SetRotation(glm::quat(glm::radians(glm::vec3(0, 45, 0))));
child->SetScale(glm::vec3(0.5f));
child->SetDrawable(myModel);
parent->AddChild(child);

// child's world position = parent.worldTransform * child.localTransform
```

### 2D + 3D Hybrid

The 3D pipeline draws first, then you can overlay 2D content on top:

```cpp
// In update loop:
renderer.Render(scene);       // 3D scene
m_2DRoot.Update();            // 2D overlay (Util::Renderer)
```

### Configuration (config.json)

New 3D settings in `config.json`:

```json
{
    "default_fov": 45.0,
    "near_clip_3d": 0.1,
    "far_clip_3d": 1000.0,
    "shadow_map_size": 2048,
    "enable_pbr": true,
    "enable_shadows": true,
    "max_point_lights": 16,
    "max_spot_lights": 8
}
```

### Shader Files

| Shader | Description |
|:-------|:------------|
| `Phong.vert` / `Phong.frag` | Blinn-Phong with normal mapping and PCF shadows |
| `PBR.vert` / `PBR.frag` | Cook-Torrance PBR with metallic-roughness workflow |
| `Shadow.vert` / `Shadow.frag` | Depth-only pass for shadow map generation |
| `Skybox.vert` / `Skybox.frag` | Cubemap skybox at infinite distance |
| `Skeletal.vert` | Vertex skinning with up to 128 bones |

### API Reference

Full API documentation is generated via Doxygen:
```bash
doxygen docs/Doxyfile
```
