# ZenEngine TODO List

## Physics & Collisions
- [ ] Implement collision with segments (line segments).
- [ ] Implement collision with arbitrary polygons.
- [ ] Add raycasting support for Line of Sight and shooting mechanics.
- [ ] Implement continuous collision detection (CCD) for fast-moving objects.

## Graphics & Rendering
- [ ] Support for animated sprites (SpriteFrames or AnimationPlayer).
- [ ] Spine skeletal animation integration (assets are present in `assets/spine`, need engine support).
- [ ] Expand particle system features (bursts, color gradients, velocity over lifetime).
- [ ] Camera effects (screen shake, post-processing shaders).

## Engine Core
- [ ] Scene serialization (saving and loading scenes to/from disk).
- [ ] Implement Audio System (wrap Raylib's audio functions for sound effects and music).
- [ ] Input Action mapping system (e.g., binding multiple keys/buttons to an action like "jump").
- [ ] Scripting support or hot-reloading for C++ game logic.

## UI & HUD
- [ ] Basic UI elements (Buttons, Labels, Panels, Progress Bars).
- [ ] UI anchoring and layout management system.
- [ ] Font rendering and text alignment options.

## Optimization & Tooling
- [ ] Memory pooling for frequently created/destroyed nodes (e.g., bullets, particles).
- [ ] In-game debug console or visual editor inspector.


🔴 Alta	AnimatedSprite2D	Qualquer jogo com sprites animados (spritesheet)
🔴 Alta	RigidBody2D	Física com gravidade/impulsos
🔴 Alta	Timer	Cooldowns, delays, spawn
🟡 Média	Tween	Animações de propriedades (UI, câmera)
🟡 Média	RayCast2D	Line-of-sight, shooting
🟡 Média	Button / Label (UI)	HUDs, menus
🟡 Média	AudioStreamPlayer	Som/música (raylib já tem)
🟢 Baixa	Line2D / Polygon2D	Debug/gizmos
🟢 Baixa	Scene instancing (.zen files)	