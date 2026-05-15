# ZenEngine

A lightweight 2D game engine written in **C++17**, inspired by Godot's node-based architecture. Built on top of [Raylib](https://www.raylib.com/) for windowing, rendering and input. Designed to be embedded with a scripting VM (zenpy).

---

## Features

### Node System
- Godot-style node hierarchy — parent owns children, cascading transforms
- Lifecycle: `_ready()` → `_update(dt)` → `_draw()` → `_on_destroy()`
- `get_node("path/to/child")` — Godot-style path navigation
- Groups: `add_to_group`, `is_in_group`, `SceneTree::call_group`
- Signal system: `connect` / `emit_signal` / `disconnect`
- `ScriptHost` interface — attach any scripting VM to any node

### 2D Transform
- Local position, rotation (degrees), scale, pivot, skew
- Lazy-cached global transform (per-frame revision counter)
- `to_global` / `to_local` / `set_global_position` / `look_at`
- `move_toward`, `rotate_toward`, `distance_to`, `direction_to`

### Visual Nodes

| Node | Description |
|---|---|
| `Sprite2D` | Textured quad — flip, blend, pivot, clip, offset |
| `AnimatedSprite2D` | Frame-based sprite animation |
| `TextNode2D` | Text in world space |
| `TileMap2D` | Orthogonal / isometric tilemaps (.tmx) |
| `Parallax2D` + `ParallaxLayer2D` | Multi-layer parallax scrolling |
| `Line2D` | Polyline with rounded end-caps, closed option |
| `Rect2D` | Rectangle — filled or outline, respects rotation |
| `Circle2D` | Circle — filled or outline |
| `PolyMesh2D` | Arbitrary polygon mesh (poly2tri triangulation) |
| `Path2D` | Catmull-Rom spline with arc-length LUT |
| `PathFollow2D` | Node that moves along a Path2D |

### Immediate Draw Helpers (inside `_draw`)

All coordinates are **local space** — transformed to world automatically:

```cpp
draw_circle({0,0}, 32, RED);                    // filled
draw_circle({0,0}, 32, RED, false);             // outline
draw_rect({-50,-50}, {100,100}, BLUE);          // filled
draw_rect({-50,-50}, {100,100}, BLUE, false);   // outline
draw_line({-100,0}, {100,0}, WHITE, 2.f);
draw_triangle(a, b, c, GREEN);
```

### Camera

- `View2D` — zoom, follow target, dead zone, smooth lerp
- Camera shake (cyclic + trauma model), zoom punch
- `is_on_screen(world_pos, radius)` — viewport culling helper
- `world_to_screen` / `screen_to_world` / `get_viewport_rect`

### Lighting & Shadows

- `Light2D` — point and spot lights, intensity, radius, color
- `ShadowCaster2D` — per-segment shadow volumes, silhouette culling
- `Renderer2D` — full deferred pipeline:
  - `scene_rt` → world geometry
  - `shadow_rt` → per-light shadow mask (5-tap soft blur)
  - `light_rt` → additive light accumulation
  - composite shader → `scene * (ambient + lights)`
- Cross-platform GLSL (desktop 330 core, WASM 300 es, Android ES2)
- Off-screen lights skipped via viewport frustum culling
- Screen FX: `fade_in`, `fade_out`, `flash`

### Physics & Collision

| Node | Description |
|---|---|
| `CharacterBody2D` | `move_and_slide`, `move_and_collide`, floor/wall/ceiling |
| `StaticBody2D` | Immovable colliders |
| `RigidBody2D` | Physics body — impulses, gravity, friction, restitution |
| `Area2D` | Trigger zones — enter / exit / stay callbacks |
| `Collider2D` | Box, circle, capsule, convex polygon — SAT |
| `RayCast2D` | Raycast against bodies and tilemaps |
| `SpatialHash2D` | O(1) amortised broad-phase |
| `NavigationGrid2D` | A* pathfinding over TileMap2D |

- Collision layers & masks, debug contacts overlay

### Scene Management

- `SceneTree::change_scene(fn)` — instant or fade transition
- `queue_free` — deferred node deletion (safe during update)
- World pass / UI pass / overlay pass separation

### Utilities

| | |
|---|---|
| `Timer` | One-shot and repeat timers |
| `Tween` | Property interpolation — linear, ease, bounce, elastic |
| `ParticleEmitter2D` | CPU particle system |
| `Assets` / `GraphLib` | Texture + metadata library, numeric graph IDs |
| `Input` | Actions, keyboard, mouse, gamepad — pressed/held/released |
| `SaveData` | Key-value file persistence (raylib cross-platform I/O) |
| `SceneSerializer` | Save/load scene graph as XML (tinyxml2 + raylib I/O) |

---

## Project Structure

```
zenengine/
  include/      public headers
  src/          implementation, pch.hpp, tinyxml2
main/
  src/          test applications (one per feature)
assets/         sprites, tilemaps, spine data
vendor/
  poly2tri/     polygon triangulation
  box2d/        available, not yet integrated
build/          cmake output
bin/            compiled executables
```

---

## Building

Requirements: **C++17** compiler, **CMake ≥ 3.16**, **Raylib** installed system-wide.

```bash
git clone https://github.com/akadjoker/zengame
cd zengame
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

Binaries are placed in `bin/`.

### Running tests

```bash
./bin/test_characterbody2d
./bin/test_light2d
./bin/test_path2d
./bin/test_rigidbody_rotation
./bin/test_platformer2d
```

---

## Quick Start

```cpp
#include "SceneTree.hpp"
#include "View2D.hpp"
#include "CharacterBody2D.hpp"
#include "Collider2D.hpp"

class Player : public CharacterBody2D {
public:
    explicit Player(const std::string& n) : CharacterBody2D(n) {}

    void _update(float dt) override {
        Vec2 vel = {};
        if (IsKeyDown(KEY_RIGHT)) vel.x =  200;
        if (IsKeyDown(KEY_LEFT))  vel.x = -200;
        if (IsKeyDown(KEY_UP))    vel.y = -300;
        velocity = vel;
        move_and_slide(dt);
    }

    void _draw() override {
        draw_rect({-16,-24}, {32,48}, GREEN, false);
    }
};

int main() {
    InitWindow(1280, 720, "ZenEngine");
    SetTargetFPS(60);

    SceneTree tree;

    auto* root = new Node2D("Root");

    auto* cam = new View2D("Camera");
    cam->is_current = true;
    root->add_child(cam);

    auto* player = new Player("Player");
    player->position = {640, 360};
    auto* col = new Collider2D("Col");
    col->set_box(32, 48);
    player->add_child(col);
    root->add_child(player);

    tree.set_root(root);
    tree.start();

    while (!WindowShouldClose()) {
        tree.process(GetFrameTime());
        BeginDrawing();
        ClearBackground(BLACK);
        tree.draw();
        EndDrawing();
    }

    CloseWindow();
}
```

---

## License

See `LICENSE`.
