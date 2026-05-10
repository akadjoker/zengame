# ZenEngine (ZenGame)

ZenEngine is a 2D game engine written in C++17, heavily inspired by the node-based architecture and lifecycle of Godot Engine. It is built on top of [Raylib](https://www.raylib.com/) for fast rendering, window management, and input, offering a lightweight and flexible framework for 2D game development in C++.

## Features

### 🌳 Node-Based Architecture
The core of ZenEngine revolves around a hierarchical node system, closely mirroring Godot's design:
- **`Node` / `Node2D`**: Base classes for all objects in the game. They support parent-child relationships where transformations are inherited.
- **Lifecycle Methods**: Override `_ready()`, `_update(dt)`, and `_draw()` to implement game logic. 
- **`SceneTree`**: Manages the active node tree, game loop updates, and rendering passes.

### ⚙️ 2D Physics & Collisions
A custom 2D physics and collision detection system tailored for platformers and top-down games:
- **`CharacterBody2D`**: For player characters and enemies requiring kinetic movement and collision resolution.
- **`Area2D`**: For triggers and overlap detection.
- **Spatial Hashing (`SpatialHash2D`)**: Efficient broad-phase collision detection.

### 🎨 Graphics & Rendering
Rich 2D rendering capabilities out-of-the-box:
- **Lighting System**: Dynamic 2D lighting with `Light2D` and 2D shadow casting via `ShadowCaster2D`.
- **Parallax Backgrounds**: Easy multi-layered scrolling backgrounds using `Parallax2D`.
- **Particles**: Flexible particle system through `ParticleEmitter2D`.
- **Tilemaps**: Integration for rendering level tilemaps.
- **Polygon Meshes**: `PolyMesh2D` with integration of `poly2tri` for polygon triangulation and complex shapes.

## Dependencies

- **C++17** compiler
- **CMake** (3.16 or higher)
- **Raylib**: Required for windowing, input, audio, and basic rendering.
- **Poly2Tri**: Included in `vendor/poly2tri` for polygon triangulation.

## Project Structure

- `zenengine/` - Engine source code and headers (`include/` and `src/`).
- `main/` - Example test applications demonstrating various engine features (`test_sprite.cpp`, `test_physics.cpp`, etc.).
- `assets/` - Game assets used by the examples (sprites, tilemaps, spine data).
- `vendor/` - Third-party libraries (e.g., Poly2Tri).
- `port/` - Legacy/porting experimental engine components.

## Building the Project

The engine uses CMake for building. Ensure you have Raylib installed on your system or available in your CMake prefix path.

```bash
# Clone the repository
git clone [<repository-url>](https://github.com/akadjoker/zengame
cd zengame

# Create build directory
mkdir build
cd build

# Configure and compile
cmake ..
make
```

### Running Examples
After building, the executables will be located in the `bin/` directory. You can run the various test applications to see the engine in action:
```bash
../bin/test_characterbody2d
../bin/test_light2d
../bin/test_particles2d
```

## License
*(Please refer to the repository's license file if available, or update this section accordingly.)*
