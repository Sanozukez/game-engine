# AI Assistant Instructions for Game Engine Codebase

## Language Preference
**Always respond in Portuguese (Brazil) - pt-BR** unless explicitly asked to use another language. Code comments, variable names, and technical terms should remain in English following industry standards, but explanations, discussions, and documentation should be in Portuguese.

## Project Overview
Modular C++ game engine for 2.5D/3D games (Rohan/MU Online style). Uses OpenGL with GLFW + GLAD + GLM. Architecture follows Single Responsibility Principle (SRP) with strict modularization.

**C++ Standard**: C++20 (requires `std::format`, `std::source_location`)
**Graphics API**: OpenGL 3.3 Core
**Build System**: CMake 3.25+
**Dependencies**: Managed via FetchContent (see `cmake/dependencies.cmake`)

## Critical Architecture Patterns

### ECS (Entity Component System)
- **World**: Central registry (`engine/ecs/world.h`)
  - Manages entities, components via `IComponentManager` interface (OCP/DIP)
  - Systems registered with `ComponentSignature` for automatic entity matching
- **Systems**: Inherit from `BaseSystem`, implement `update(World& world, float dt)`
  - Example systems: `AnimationSystem`, `RenderSystem`, `PlayerSystem`
- **Components**: POD structs in `engine/ecs/components/`
  - Must be registered in World with `registerComponent<T>()`

### Animation Pipeline (CRITICAL - Complex Flow)
**Asset Loading** (Offline):
1. `GLTFLoader::loadGLTF()` → parses GLTF with cgltf
2. `AnimationDataMapper::mapAnimationData()` → extracts skeleton + animations
   - `processBoneNode()`: Builds bone hierarchy (parent IDs)
   - Maps keyframes (T/R/S) per bone channel
   - Stores rest pose transforms in `Model::m_nodeTransforms`
3. Result: `Model` with `Skeleton` + `AnimationAsset` data

**Runtime** (Every frame):
1. `AnimationSystem::update()` per animated entity:
   - Load rest pose → `boneFinalLocalTransforms[boneId] = restPoseTransform`
   - Apply animation channels → interpolate T/R/S via `KeyframeSampler`
   - `ForwardKinematics::computeGlobalTransforms()` → parent * child
   - Apply IBM: `finalTransform = globalTransform * bone.inverseBindMatrix`
2. Upload to GPU: `uBoneTransforms[MAX_BONES]` uniform array
3. Vertex shader (`animated.vert`):
   ```glsl
   mat4 skin = aWeights.x * uBoneTransforms[aBoneIDs.x] + ...;
   vec4 pos = skin * vec4(aPos, 1.0);
   ```

**CRITICAL: Coordinate System Workaround (TESTED & WORKING)**:
After 50+ configuration tests, discovered that **ONLY X=180° rotation maintains bone structure**.
All other rotations (±90° any axis, Y=180°, Z=180°, combinations) cause complete skeleton collapse.

**Root Cause**: GLTF uses Y-up Right-Handed (Blender default), but engine's animation pipeline interprets 
data in a way that requires X=180° correction to maintain humanoid structure. This is NOT a camera issue 
(camera was independently fixed from Left→Right-Handed).

**Working Configuration** (DO NOT CHANGE unless re-exporting model with correct coordinate system):

1. **Animation Data Loading** (`engine/asset/animation_data_mapper.cpp`):
   - Line 342: Apply X=180° to rotation keyframes
     ```cpp
     glm::quat coordinateCorrection = glm::angleAxis(glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
     rotationValue = coordinateCorrection * rotationValue;
     ```
   - Line 175: Apply X=180° to Inverse Bind Matrices
     ```cpp
     glm::mat4 coordinateCorrection = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
     ibm = ibm * glm::inverse(coordinateCorrection);
     ```
   - **Side Effect**: X=180° inverts Y-axis (upside-down) AND Z-axis (backwards)

2. **Player Transform Setup** (`src/app/app_setup.cpp` lines 143-154):
   - **Scale Y=-1**: Flips upside-down character back to standing
     ```cpp
     playerTransform.scale = Engine::Math::Vec3(1.0f, -1.0f, 1.0f);
     ```
   - **Position Y negated**: Compensates for Scale Y=-1 coordinate inversion
     ```cpp
     correctedPos.y = -correctedPos.y;
     ```
   - **Rotation IDENTITY**: Initial forward-facing (no Y=180° needed)
     ```cpp
     playerTransform.rotation = Engine::Math::Quat(glm::identity<glm::quat>());
     ```

3. **Player Movement Controls** (`engine/ecs/systems/player_system.cpp`):
   - **WASD Z-inversion** (lines 136-137): Compensates for Z-axis flip from X=180°
     ```cpp
     camF.z = -camF.z;
     camR.z = -camR.z;
     ```
   - **Q/E Strafe inversion** (lines 151-154): Q=right, E=left (inverted)
     ```cpp
     if (GLFW_KEY_Q) dir += camR;  // Normally would be -= camR
     if (GLFW_KEY_E) dir -= camR;  // Normally would be += camR
     ```
   - **RMB+A/D Strafe inversion** (lines 160-163): A=right, D=left (inverted)
     ```cpp
     if (GLFW_KEY_A) dir += camR;  // Normally would be -= camR
     if (GLFW_KEY_D) dir -= camR;  // Normally would be += camR
     ```

4. **Terrain Tracking** (`engine/ecs/systems/terrain_tracking_system.cpp` lines 108-112):
   - **Height compensation** for Scale Y=-1:
     ```cpp
     if (transform.scale.y < 0.0f) {
         transform.position.y = -groundHeight + 1.5f; // +1.5f = root bone height offset
     }
     ```

5. **Camera System** (`engine/camera/orbit_camera.cpp` line 89):
   - **Independent Fix**: Changed from Left-Handed to Right-Handed
     ```cpp
     return glm::vec3(sin(yaw), 0, -cos(yaw)); // Note: -cos for Right-Handed
     ```
   - This fix is INDEPENDENT from animation workaround (both required)

**Verification**:
- ✅ Model displays correctly in external GLTF viewers (McCurdy, Blender) - proves model data is correct
- ✅ Character stands upright, faces forward, moves correctly
- ✅ All controls work: WASD, Q/E, RMB+A/D, Click-To-Move
- ✅ Bone structure maintained (no collapse)
- ✅ Terrain tracking functional

**Long-term Solution**: Re-export model from Blender with engine's expected coordinate system to eliminate workarounds.

**Common Animation Bugs**:
- **Bone collapse**: Check bone weights (`WEIGHTS_0`) not zero in GLTF
  - The correction is MANDATORY - adjust axis/angle, don't remove it
- **Inverted/upside-down skeleton**: 
  - Verify coordinate correction applied to BOTH rotation keyframes AND IBMs
  - Check `animation_data_mapper.cpp` lines 169 (IBM) and 341 (keyframes)
  - Currently using 180° X-axis rotation: `glm::angleAxis(radians(180.0f), vec3(1,0,0))`
  - If upside-down, try X-axis; if backwards, try Y-axis; if sideways, try Z-axis
- **Wrong hierarchy**: Log `parentId` in `processBoneNode()` - must form tree
- **Root bone behavior**: Root stays at (0,0,0), never animated, no mesh weights (GLTF best practice)

### Asset Management
- **AssetManager**: Singleton with model cache (`m_modelCache`)
  - Uses asset dictionary (`data/asset_dictionary.json`) for ID → path mapping
  - `getModel(uint32_t assetID)` returns cached `shared_ptr<Model>`
- **Model**: Contains meshes, skeleton, animations, node transforms
  - `getNodeLocalTransform(boneName)` retrieves rest pose
  - Animations accessed via `getAnimation(animID)`

### Coordinate Systems
- **GLTF**: Y-up, right-handed (Blender default)
- **Engine**: Applies 180° X-axis rotation correction (horizontal flip)
- **Transform order**: T * R * S (GLTF standard)
- **CRITICAL**: Coordinate correction applied in TWO places:
  1. **Animation keyframes**: `animation_data_mapper.cpp:341` - Rotation quaternions
  2. **Inverse Bind Matrices**: `animation_data_mapper.cpp:169` - IBM correction
  3. Both use `glm::angleAxis(radians(180.0f), vec3(1,0,0))` (X-axis rotation)
- ⚠️ **If model is upside-down/backwards**: Check if correction applied to BOTH keyframes AND IBMs
- ⚠️ **Bone collapse without keyframe correction**: Keyframe correction is MANDATORY for proper skeleton structure

## Build & Debug

### Commands (PowerShell)
```powershell
# Initial build
cmake -B build
cmake --build build

# Run
cd build/src/Debug
./game-engine.exe

# Rebuild after changes
cmake --build build --target game-engine
```

### Debugging Animation Issues
1. Enable trace logging in `src/app/app.cpp`:
   ```cpp
   Engine::Core::Log::SetLogLevel(Engine::Core::LogLevel::Trace);
   ```
2. Key log tags to search:
   - `[FK_DEBUG]`: Forward kinematics bone transforms
   - `[ANIM_CHANNEL]`: Keyframe interpolation
   - `[REST_POSE_TEST]`: Rest pose loading
   - `[FINAL_TRANSFORM]`: Final matrix after IBM
3. Check `animation_system.cpp` lines 120-160 for detailed bone logs

### Common Development Errors
- **Missing weights in GLTF**: See `gltf_data_reader.cpp:85` - critical warning about `WEIGHTS_0`
- **IBM (Inverse Bind Matrix) incorrect**: 
  - Verify coordinate correction applied: `IBM_corrected = IBM_original * inverse(correction)`
  - Formula: `ibm * glm::inverse(coordinateCorrection)` in `animation_data_mapper.cpp:176`
  - Root bone IBM should transform (0,0,0) → (0,0,0) after correction
- **Parent hierarchy broken**: Use `processBoneNode()` logs to trace tree structure
- **Shader uniform errors**: Max 100 bones (`MAX_BONES` in `animated.vert`)
- **Bones pointing wrong direction**: Check Local Y-axis in FK_DEBUG logs - should point up (positive Y) for spine bones

## File Organization

```
engine/
├── animation/        # Runtime: FK, keyframe sampling, pose utils
│   ├── forward_kinematics.cpp
│   └── keyframe_sampler.cpp
├── asset/           # Loading: GLTF parser, animation mapper
│   ├── gltf_loader.cpp
│   ├── gltf_data_reader.cpp  # Mesh attributes (weights!)
│   ├── animation_data_mapper.cpp  # Skeleton/animation extraction
│   └── asset_manager.h       # Singleton cache
├── ecs/
│   ├── systems/     # AnimationSystem, RenderSystem, PlayerSystem
│   ├── components/  # AnimationComponent, Mesh, Transform
│   └── world.h      # Central ECS registry
├── render/          # OpenGL renderer, shaders, materials
└── shaders/         # GLSL files (animated.vert for skinning)
```

## Naming Conventions
- **Namespaces**: `Engine::ECS::System`, `Engine::Asset`, `Engine::Animation`
- **Known naming debt** (see `docs/need_to_refactor.md`):
  - `Animation` (component) should be `AnimationComponent`
  - `Animation` (asset) should be `AnimationAsset`
  - Current code uses both - check context carefully!

## Logging System
```cpp
Engine::Core::Log::Info("Message");   // Normal operations
Engine::Core::Log::Warn("Message");   // Non-critical issues
Engine::Core::Log::Error("Message");  // Recoverable errors
Engine::Core::Log::Critical("Message"); // Fatal errors
// Use std::format for formatting: std::format("Value: {}", x)
```

## Development Workflow
1. **Adding a component**: Create in `engine/ecs/components/`, register in World
2. **Adding a system**: Inherit `BaseSystem`, define required `ComponentSignature`
3. **Modifying shaders**: Edit in `engine/shaders/`, auto-loaded by `ShaderManager`
4. **Adding assets**: Update `data/asset_dictionary.json`, use `AssetManager::getModel()`

## Performance Notes
- Bone transforms: Max 100 per mesh (GPU uniform limit)
- Asset cache: Models loaded once, shared via `shared_ptr`
- ECS iteration: Systems only update entities with matching signatures