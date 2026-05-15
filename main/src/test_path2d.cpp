// test_path2d.cpp
//
// Three demos on a single screen:
//
//  LEFT PANEL  — Closed loop patrol (enemy icon follows a figure-8 spline)
//  MIDDLE PANEL — One-shot homing bullet (PathFollow2D, loop=false)
//  RIGHT PANEL  — Moving platform (h_offset + vertical sinusoidal path)
//
// Controls:
//   SPACE  — toggle debug overlay on all paths
//   R      — reset offsets
//   LEFT/RIGHT arrows — manually scrub the selected follower

#include "pch.hpp"
#include <raylib.h>
#include <cmath>
#include <string>

#include "SceneTree.hpp"
#include "Node2D.hpp"
#include "Path2D.hpp"
#include "PathFollow2D.hpp"
#include "Input.hpp"

static constexpr int SW = 1280;
static constexpr int SH = 720;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
static void label(const char* text, float x, float y, Color c = RAYWHITE)
{
    DrawText(text, (int)x, (int)y, 18, c);
}

static void draw_panel_title(const char* text, float cx)
{
    const int w = MeasureText(text, 22);
    DrawText(text, (int)(cx - w * 0.5f), 16, 22, SKYBLUE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Patrol enemy (follow closed loop, rotate=true)
// ─────────────────────────────────────────────────────────────────────────────
class PatrolEnemy : public Node2D
{
    int m_laps = 0;
public:
    explicit PatrolEnemy(const std::string& n) : Node2D(n) {}

    void _draw() override
    {
        Node2D::_draw();
        const Vec2 p = get_global_position();
        // Body
        DrawCircleV({p.x, p.y}, 14, Color{200, 80, 80, 255});
        // Eye — use rotation to look forward
        const float rad = rotation * (3.14159265f / 180.0f);
        const Vec2 eye{p.x + cosf(rad) * 8, p.y + sinf(rad) * 8};
        DrawCircleV({eye.x, eye.y}, 4, WHITE);
        DrawCircleV({eye.x, eye.y}, 2, BLACK);

        const std::string info = "laps: " + std::to_string(m_laps);
        label(info.c_str(), p.x - 22, p.y + 18, LIGHTGRAY);
    }

    void increment_laps() { ++m_laps; }
};

// ─────────────────────────────────────────────────────────────────────────────
// Bullet (one-shot, disappears at path end)
// ─────────────────────────────────────────────────────────────────────────────
class Bullet : public Node2D
{
public:
    bool alive = true;
    explicit Bullet(const std::string& n) : Node2D(n) {}

    void _draw() override
    {
        if (!alive) return;
        Node2D::_draw();
        const Vec2 p = get_global_position();
        const float rad = rotation * (3.14159265f / 180.0f);
        const Vec2 tip{p.x + cosf(rad) * 12, p.y + sinf(rad) * 12};
        DrawLineEx({p.x, p.y}, {tip.x, tip.y}, 4, ORANGE);
        DrawCircleV({p.x, p.y}, 5, YELLOW);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Moving platform
// ─────────────────────────────────────────────────────────────────────────────
class Platform : public Node2D
{
public:
    explicit Platform(const std::string& n) : Node2D(n) {}
    void _draw() override
    {
        Node2D::_draw();
        const Vec2 p = get_global_position();
        DrawRectangle((int)(p.x - 50), (int)(p.y - 10), 100, 20, Color{80, 160, 255, 230});
        DrawRectangleLines((int)(p.x - 50), (int)(p.y - 10), 100, 20, WHITE);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Demo scene
// ─────────────────────────────────────────────────────────────────────────────
class DemoScene : public Node2D
{
    // ── Left panel: closed patrol ────────────────────────────────────────────
    Path2D*       m_patrol_path     = nullptr;
    PathFollow2D* m_patrol_follower = nullptr;
    PatrolEnemy*  m_enemy           = nullptr;

    // ── Middle panel: one-shot bullet ───────────────────────────────────────
    Path2D*       m_bullet_path     = nullptr;
    PathFollow2D* m_bullet_follower = nullptr;
    Bullet*       m_bullet          = nullptr;
    float         m_bullet_reload   = 0.0f;

    // ── Right panel: platform ───────────────────────────────────────────────
    Path2D*       m_plat_path       = nullptr;
    PathFollow2D* m_plat_follower   = nullptr;

    bool m_show_debug  = true;   // on by default
    float m_scrub_t    = 0.0f;

public:
    explicit DemoScene() : Node2D("DemoScene") {}

    void _ready() override
    {
        Input::map("debug",   {KEY_SPACE});
        Input::map("reset",   {KEY_R});
        Input::map("scrub_l", {KEY_LEFT});
        Input::map("scrub_r", {KEY_RIGHT});

        build_patrol();
        build_bullet();
        build_platform();
    }

    // ── Left panel: oval closed loop ─────────────────────────────────────────
    void build_patrol()
    {
        const float cx = SW * 0.16f;
        const float cy = SH * 0.50f;

        m_patrol_path = new Path2D("PatrolPath");
        m_patrol_path->loop        = true;
        m_patrol_path->show_debug  = true;   // visible by default
        m_patrol_path->debug_color = {160, 240, 140, 200};
        m_patrol_path->position    = {cx, cy};

        // Simple oval — smooth tangents, no Catmull-Rom overshoot
        const float rx = 150, ry = 110;
        const int   N  = 10;
        for (int i = 0; i < N; ++i)
        {
            const float t  = i * (2.0f * 3.14159265f / N);
            m_patrol_path->add_point({cosf(t) * rx, sinf(t) * ry});
        }
        add_child(m_patrol_path);

        m_patrol_follower = new PathFollow2D("EnemyFollower");
        m_patrol_follower->speed  = 220.0f;
        m_patrol_follower->rotate = true;
        m_patrol_follower->loop   = true;
        m_patrol_path->add_child(m_patrol_follower);

        m_enemy = new PatrolEnemy("Enemy");
        m_patrol_follower->add_child(m_enemy);

        m_patrol_follower->on_loop_completed = [this]{
            m_enemy->increment_laps();
        };
    }

    // ── Middle panel: arcing bullet path (open, no loop) ─────────────────────
    void build_bullet()
    {
        const float cx = SW * 0.50f;
        const float cy = SH * 0.52f;

        m_bullet_path = new Path2D("BulletPath");
        m_bullet_path->loop       = false;
        m_bullet_path->show_debug = true;   // visible by default
        m_bullet_path->debug_color = {255, 180, 80, 200};
        m_bullet_path->position   = {cx, cy};

        // Gentle arc left→right
        m_bullet_path->add_point({-200,  80});
        m_bullet_path->add_point({ -80,  10});
        m_bullet_path->add_point({   0, -50});
        m_bullet_path->add_point({  80,  10});
        m_bullet_path->add_point({ 200,  80});
        add_child(m_bullet_path);

        spawn_bullet();
    }

    void spawn_bullet()
    {
        m_bullet_follower = new PathFollow2D("BulletFollower");
        m_bullet_follower->speed  = 160.0f;
        m_bullet_follower->rotate = true;
        m_bullet_follower->loop   = false;
        m_bullet_path->add_child(m_bullet_follower);

        m_bullet = new Bullet("Bullet");
        m_bullet_follower->add_child(m_bullet);

        m_bullet_follower->on_reached_end = [this]{
            m_bullet->alive = false;
            m_bullet_reload = 1.0f;
        };
    }

    void reset_bullet()
    {
        // Reset the existing follower instead of spawning (avoids leaks)
        m_bullet->alive           = true;
        m_bullet_follower->offset = 0.0f;
        m_bullet_follower->speed  = 160.0f;
        m_bullet_reload           = 0.0f;
    }

    // ── Right panel: vertical bouncing platform ───────────────────────────────
    void build_platform()
    {
        const float cx = SW * 0.83f;
        const float cy = SH * 0.50f;
        const float travel = 210.0f;

        m_plat_path = new Path2D("PlatformPath");
        m_plat_path->loop        = true;
        m_plat_path->show_debug  = true;   // visible by default
        m_plat_path->debug_color = {100, 180, 255, 180};
        m_plat_path->position    = {cx, cy};

        // Straight vertical path (Catmull-Rom will pass through directly)
        const int pts = 6;
        for (int i = 0; i < pts; ++i)
        {
            const float y = -travel + (2.0f * travel / (pts - 1)) * i;
            m_plat_path->add_point({0, y});
        }
        add_child(m_plat_path);

        m_plat_follower = new PathFollow2D("PlatFollower");
        m_plat_follower->speed    = 130.0f;
        m_plat_follower->rotate   = false;
        m_plat_follower->loop     = true;
        m_plat_path->add_child(m_plat_follower);

        Platform* plat = new Platform("Platform");
        m_plat_follower->add_child(plat);
    }

    // ── _update ────────────────────────────────────────────────────────────────
    void _update(float dt) override
    {
        Node2D::_update(dt);

        // Toggle debug overlay
        if (Input::is_action_just_pressed("debug"))
        {
            m_show_debug = !m_show_debug;
            m_patrol_path->show_debug = m_show_debug;
            m_bullet_path->show_debug = m_show_debug;
            m_plat_path->show_debug   = m_show_debug;
        }

        // Reset offsets
        if (Input::is_action_just_pressed("reset"))
        {
            m_patrol_follower->offset = 0.0f;
            m_plat_follower->offset   = 0.0f;
        }

        // Manual scrub of patrol follower
        if (Input::is_action_held("scrub_l")) m_patrol_follower->offset -= 150.0f * dt;
        if (Input::is_action_held("scrub_r")) m_patrol_follower->offset += 150.0f * dt;

        // Bullet respawn — reset offset, no allocation
        if (m_bullet_reload > 0.0f)
        {
            m_bullet_reload -= dt;
            if (m_bullet_reload <= 0.0f)
                reset_bullet();
        }
    }

    // ── _draw ─────────────────────────────────────────────────────────────────
    void _draw() override
    {
        ClearBackground(Color{15, 18, 28, 255});

        // Panel dividers
        DrawLineEx({(float)(SW/3), 0}, {(float)(SW/3), (float)SH}, 1, Color{60,60,80,200});
        DrawLineEx({(float)(2*SW/3), 0}, {(float)(2*SW/3), (float)SH}, 1, Color{60,60,80,200});

        // Titles
        draw_panel_title("Closed Loop Patrol",  SW * 0.16f);
        draw_panel_title("One-Shot Bullet Path", SW * 0.50f);
        draw_panel_title("Moving Platform",      SW * 0.83f);

        // HUD
        const float path_len = m_patrol_path ? m_patrol_path->get_total_length() : 0.0f;
        const float off      = m_patrol_follower ? m_patrol_follower->offset : 0.0f;
        const float prog     = m_patrol_follower ? m_patrol_follower->get_progress() * 100.0f : 0.0f;

        label("SPACE = toggle debug overlay   R = reset offsets   LEFT/RIGHT = scrub patrol",
              16, SH - 36, DARKGRAY);

        const std::string stat =
            "patrol path len: " + std::to_string((int)path_len) +
            "   offset: " + std::to_string((int)off) +
            "   progress: " + std::to_string((int)prog) + "%";
        label(stat.c_str(), 16, SH - 58, GRAY);

        // Let the scene tree draw children (paths + followers + entities)
        Node2D::_draw();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main()
{
    InitWindow(SW, SH, "ZenEngine - Path2D + PathFollow2D demo");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   // disable ESC closing window

    SceneTree tree;
    DemoScene* demo = new DemoScene();
    tree.set_root(demo);
    tree.start();

    while (!WindowShouldClose())
    {
        Input::update();
        tree.process(GetFrameTime());

        BeginDrawing();
        tree.draw();
        EndDrawing();
    }


   
    CloseWindow();
    return 0;
}
