// test_motionstreak.cpp
//
// Visual test for MotionStreak2D.
//
// Demos:
//   LEFT  — "Sword swing": streak follows a node orbiting in a tight circle,
//           simulates a weapon trail (short fade, wide stroke)
//   MIDDLE — "Projectile": streak follows a bouncing node across the panel,
//           thin fast trail
//   RIGHT  — "Mouse trail": streak attached to mouse cursor, any color
//
// Controls:
//   1/2/3  — change trail color preset
//   UP/DOWN — adjust stroke width
//   R       — reset all streaks
//   SPACE   — pause/resume orbit animations

#include "pch.hpp"
#include <raylib.h>
#include <cmath>
#include <string>

#include "SceneTree.hpp"
#include "Node2D.hpp"
#include "MotionStreak2D.hpp"

static constexpr int SW = 1280;
static constexpr int SH = 720;

static void label(const char* text, float x, float y, Color c = RAYWHITE)
{
    DrawText(text, (int)x, (int)y, 18, c);
}

static void panel_title(const char* text, float cx)
{
    const int w = MeasureText(text, 22);
    DrawText(text, (int)(cx - w * 0.5f), 16, 22, SKYBLUE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Orbiting emitter — moves in a circle each frame
// ─────────────────────────────────────────────────────────────────────────────
class OrbiterNode : public Node2D
{
public:
    Vec2  center;
    float radius  = 80.f;
    float speed   = 240.f;   // degrees/sec
    float angle   = 0.f;     // current angle in degrees
    bool  paused  = false;

    OrbiterNode() : Node2D("OrbiterNode") {}
    explicit OrbiterNode(const std::string& n) : Node2D(n) {}

    void _update(float dt) override
    {
        Node2D::_update(dt);
        if (!paused)
            angle += speed * dt;
        const float rad = angle * (M_PI_F / 180.f);
        position = {center.x + cosf(rad) * radius,
                    center.y + sinf(rad) * radius};
    }

    void _draw() override
    {
        Node2D::_draw();
        const Vec2 p = get_global_position();
        DrawCircleV({p.x, p.y}, 7.f, YELLOW);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Bouncing emitter — bounces horizontally inside a panel
// ─────────────────────────────────────────────────────────────────────────────
class BouncerNode : public Node2D
{
public:
    float left_x, right_x;
    float vel_x   = 280.f;
    float center_y;
    bool  paused  = false;

    BouncerNode() : Node2D("BouncerNode"), left_x(0.f), right_x(0.f), center_y(0.f) {}
    explicit BouncerNode(const std::string& n) : Node2D(n), left_x(0.f), right_x(0.f), center_y(0.f) {}

    void _update(float dt) override
    {
        Node2D::_update(dt);
        if (paused) return;
        position.x += vel_x * dt;
        if (position.x > right_x)  { position.x = right_x;  vel_x = -vel_x; }
        if (position.x < left_x)   { position.x = left_x;   vel_x = -vel_x; }
        position.y = center_y + sinf(position.x * 0.04f) * 60.f;
    }

    void _draw() override
    {
        Node2D::_draw();
        const Vec2 p = get_global_position();
        DrawCircleV({p.x, p.y}, 6.f, ORANGE);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Mouse emitter — follows mouse cursor
// ─────────────────────────────────────────────────────────────────────────────
class MouseNode : public Node2D
{
public:
    MouseNode() : Node2D("MouseNode") {}
    explicit MouseNode(const std::string& n) : Node2D(n) {}

    void _update(float dt) override
    {
        Node2D::_update(dt);
        const Vector2 mp = GetMousePosition();
        position = {mp.x, mp.y};
    }

    void _draw() override
    {
        Node2D::_draw();
        const Vec2 p = get_global_position();
        DrawCircleV({p.x, p.y}, 5.f, WHITE);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

static const Color COLOR_PRESETS[3] = {
    {255, 180,  50, 255},   // golden sword
    { 80, 200, 255, 255},   // blue laser
    {200,  80, 200, 255},   // purple magic
};

int main()
{
    InitWindow(SW, SH, "MotionStreak2D test");
    SetTargetFPS(60);

    SceneTree tree;
    auto* root = tree.create<Node2D>("root");
    tree.set_root(root);

    // ── Left panel: sword swing ────────────────────────────────────────────
    const float lx = SW / 6.f;
    auto* orbiter = tree.create<OrbiterNode>("orbiter");
    root->add_child(orbiter);
    orbiter->center = {lx, SH * 0.5f};
    orbiter->radius = 90.f;
    orbiter->speed  = 300.f;

    // Streak is a child of the orbiter so it inherits world position
    auto* sword_trail = tree.create<MotionStreak2D>("sword_trail");
    root->add_child(sword_trail);   // sibling, not child — we'll sync manually after process
    sword_trail->time_to_fade = 0.25f;
    sword_trail->stroke_width = 22.f;
    sword_trail->min_seg      = 4.f;
    sword_trail->color        = COLOR_PRESETS[0];

    // ── Middle panel: projectile ───────────────────────────────────────────
    const float mx = SW * 0.5f;
    auto* bouncer = tree.create<BouncerNode>("bouncer");
    root->add_child(bouncer);
    bouncer->left_x   = mx - 160.f;
    bouncer->right_x  = mx + 160.f;
    bouncer->center_y = SH * 0.5f;
    bouncer->position = {mx, SH * 0.5f};

    auto* proj_trail = tree.create<MotionStreak2D>("proj_trail");
    root->add_child(proj_trail);
    proj_trail->time_to_fade = 0.4f;
    proj_trail->stroke_width = 10.f;
    proj_trail->min_seg      = 5.f;
    proj_trail->color        = COLOR_PRESETS[1];

    // ── Right panel: mouse ─────────────────────────────────────────────────
    auto* mouse_node = tree.create<MouseNode>("mouse_node");
    root->add_child(mouse_node);

    auto* mouse_trail = tree.create<MotionStreak2D>("mouse_trail");
    root->add_child(mouse_trail);
    mouse_trail->time_to_fade = 0.5f;
    mouse_trail->stroke_width = 18.f;
    mouse_trail->min_seg      = 3.f;
    mouse_trail->color        = COLOR_PRESETS[2];

    // ── State ──────────────────────────────────────────────────────────────
    bool paused = false;
    int  preset = 0;

    tree.start();

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        // ── Input ──────────────────────────────────────────────────────────
        if (IsKeyPressed(KEY_SPACE))
        {
            paused = !paused;
            orbiter->paused = paused;
            bouncer->paused = paused;
        }
        if (IsKeyPressed(KEY_ONE))   preset = 0;
        if (IsKeyPressed(KEY_TWO))   preset = 1;
        if (IsKeyPressed(KEY_THREE)) preset = 2;

        if (IsKeyPressed(KEY_R))
        {
            sword_trail->reset();
            proj_trail->reset();
            mouse_trail->reset();
        }

        if (IsKeyDown(KEY_UP))
        {
            sword_trail->stroke_width += 30.f * dt;
            proj_trail->stroke_width  += 20.f * dt;
            mouse_trail->stroke_width += 25.f * dt;
        }
        if (IsKeyDown(KEY_DOWN))
        {
            sword_trail->stroke_width = std::max(2.f, sword_trail->stroke_width - 30.f * dt);
            proj_trail->stroke_width  = std::max(2.f, proj_trail->stroke_width  - 20.f * dt);
            mouse_trail->stroke_width = std::max(2.f, mouse_trail->stroke_width - 25.f * dt);
        }

        // Apply colour preset to sword + mouse trails
        sword_trail->color = COLOR_PRESETS[preset];
        mouse_trail->color = COLOR_PRESETS[preset];

        // ── Sync streak head positions to their emitters ──────────────────
        // (emitters are updated by process() above, so positions are current)
        sword_trail->position = orbiter->get_global_position();
        proj_trail->position  = bouncer->get_global_position();
        mouse_trail->position = mouse_node->get_global_position();

        // ── Update ─────────────────────────────────────────────────────────
        tree.process(dt);

        // ── Draw ───────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground({20, 20, 30, 255});

        // Panel dividers
        DrawLine(SW / 3, 0, SW / 3, SH, {60, 60, 80, 255});
        DrawLine(2 * SW / 3, 0, 2 * SW / 3, SH, {60, 60, 80, 255});

        // Panel titles
        panel_title("Sword Swing Trail",  SW / 6.f);
        panel_title("Projectile Trail",   SW * 0.5f);
        panel_title("Mouse Trail",        SW * 5.f / 6.f);

        // Orbit guide circle (left)
        DrawCircleLinesV({lx, SH * 0.5f}, orbiter->radius, {80, 80, 80, 120});

        tree.draw();

        // ── HUD ───────────────────────────────────────────────────────────
        label("SPACE=pause  R=reset  UP/DOWN=width  1/2/3=colour", 10, SH - 30, DARKGRAY);

        const std::string sw_str = "sword width: " + std::to_string((int)sword_trail->stroke_width);
        label(sw_str.c_str(), 10, SH - 55, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
