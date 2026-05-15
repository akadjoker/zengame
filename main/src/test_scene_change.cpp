// test_scene_change.cpp
//
// Demo with 3 scenes that cycle via change_scene<T>():
//
//   MenuScene   →  [ENTER]  →  GameScene
//   GameScene   →  [ESC]    →  MenuScene
//   GameScene   →  [SPACE] (all coins collected) → WinScene
//   WinScene    →  [ENTER]  →  MenuScene
//
// The three scenes live in the same file as inner Node2D subclasses.
// SceneTree owns them — no manual new/delete in the game loop.

#include "pch.hpp"
#include <raylib.h>
#include <cmath>
#include <string>
#include <vector>

#include "Assets.hpp"
#include "SceneTree.hpp"
#include "Node2D.hpp"
#include "TextNode2D.hpp"
#include "RigidBody2D.hpp"
#include "StaticBody2D.hpp"
#include "Collider2D.hpp"
#include "Area2D.hpp"
#include "View2D.hpp"
#include "Timer.hpp"
#include "Input.hpp"

static constexpr int SW = 1280;
static constexpr int SH = 720;

// forward declarations so scenes can reference each other
class MenuScene;
class GameScene;
class WinScene;

// We keep a single SceneTree for the whole process
static SceneTree* g_tree = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
// Shared drawing util
// ─────────────────────────────────────────────────────────────────────────────
static void draw_centered(const char* text, float y, int size, Color col)
{
    const int w = MeasureText(text, size);
    DrawText(text, (SW - w) / 2, (int)y, size, col);
}

// ─────────────────────────────────────────────────────────────────────────────
// MenuScene
// ─────────────────────────────────────────────────────────────────────────────
class MenuScene : public Node2D
{
public:
    int      m_high_score = 0;
    float    m_pulse      = 0.0f;

    explicit MenuScene(int high_score = 0)
        : Node2D("MenuScene"), m_high_score(high_score) {}

    void _ready() override
    {
        Input::map("confirm", {KEY_ENTER, KEY_SPACE});
    }

    void _update(float dt) override
    {
        Node2D::_update(dt);
        m_pulse += dt * 2.5f;

        if (Input::is_action_just_pressed("confirm"))
            g_tree->change_scene_fade<GameScene>(0.4f);
    }

    void _draw() override
    {
        ClearBackground(Color{10, 10, 30, 255});

        draw_centered("ZenEngine", SH * 0.22f, 72, Color{80, 180, 255, 255});
        draw_centered("Scene Change Demo", SH * 0.38f, 28, LIGHTGRAY);

        // Pulsing prompt
        const uint8_t alpha = (uint8_t)(160 + 95 * sinf(m_pulse));
        draw_centered("PRESS ENTER TO PLAY", SH * 0.60f, 32,
                      Color{255, 220, 80, alpha});

        if (m_high_score > 0)
        {
            const std::string hs = "High score: " + std::to_string(m_high_score);
            draw_centered(hs.c_str(), SH * 0.72f, 22, GOLD);
        }

        draw_centered("Collect all coins to win   ESC = menu", SH * 0.86f, 18, DARKGRAY);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Coin (Area2D trigger)
// ─────────────────────────────────────────────────────────────────────────────
class Coin : public Area2D
{
public:
    bool collected = false;
    float m_spin   = 0.0f;

    explicit Coin(const std::string& n) : Area2D(n) {}

    void _update(float dt) override
    {
        Node2D::_update(dt);
        if (!collected) m_spin += dt * 180.0f; // deg/s
    }

    void _draw() override
    {
        if (collected) return;
        const Vec2 p = get_global_position();
        const float r = 14.0f;
        DrawCircleV({p.x, p.y}, r, GOLD);
        DrawCircleLines((int)p.x, (int)p.y, r, YELLOW);
        // spin line
        const float rad = m_spin * (3.14159265f / 180.0f);
        DrawLineEx({p.x, p.y},
                   {p.x + cosf(rad) * r * 0.8f, p.y + sinf(rad) * r * 0.8f},
                   2.0f, WHITE);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Player (simple kinematic box)
// ─────────────────────────────────────────────────────────────────────────────
class Player : public Node2D
{
public:
    Vec2  vel         = {};
    int*  p_coins     = nullptr;   // points to GameScene counter
    int   m_total     = 0;         // total coins to collect
    float m_coyote    = 0.0f;
    bool  m_on_floor  = false;

    static constexpr float SPEED  = 260.0f;
    static constexpr float JUMP   = -520.0f;
    static constexpr float GRAV   = 900.0f;
    static constexpr float W      = 28.0f;
    static constexpr float H      = 36.0f;

    explicit Player(const std::string& n) : Node2D(n) {}

    void _ready() override
    {
        Input::map("left",  {KEY_LEFT,  KEY_A});
        Input::map("right", {KEY_RIGHT, KEY_D});
        Input::map("jump",  {KEY_UP,    KEY_W, KEY_SPACE});
        Input::map("menu",  {KEY_ESCAPE});
    }

    void _update(float dt) override
    {
        Node2D::_update(dt);

        // Horizontal
        float dx = Input::get_axis("left", "right");
        vel.x = dx * SPEED;

        // Gravity
        vel.y += GRAV * dt;

        // Coyote time
        if (m_on_floor) m_coyote = 0.12f;
        else            m_coyote -= dt;

        // Jump
        if (Input::is_action_just_pressed("jump") && m_coyote > 0.0f)
        {
            vel.y     = JUMP;
            m_coyote  = 0.0f;
        }

        // Integrate
        position.x += vel.x * dt;
        position.y += vel.y * dt;

        // Floor clamp (simple — real game would use CharacterBody2D)
        m_on_floor = false;
        if (position.y > SH - 80.0f - H * 0.5f)
        {
            position.y = SH - 80.0f - H * 0.5f;
            vel.y = 0.0f;
            m_on_floor = true;
        }
        // Side walls
        if (position.x < W * 0.5f)          position.x = W * 0.5f;
        if (position.x > SW - W * 0.5f)     position.x = SW - W * 0.5f;

        // Coin pickup — check overlap with sibling Coin nodes
        Node* parent = get_parent();
        if (parent && p_coins)
        {
            for (size_t i = 0; i < parent->get_child_count(); ++i)
            {
                Node* child = parent->get_child(i);
                if (!child || child == this) continue;
                auto* coin = dynamic_cast<Coin*>(child);
                if (!coin || coin->collected) continue;
                const Vec2 cp = coin->get_global_position();
                const float dx2 = position.x - cp.x;
                const float dy2 = position.y - cp.y;
                if (dx2 * dx2 + dy2 * dy2 < 40.0f * 40.0f)
                {
                    coin->collected = true;
                    (*p_coins)++;
                }
            }
        }

        invalidate_transform();
    }

    void _draw() override
    {
        const Vec2 p = get_global_position();
        DrawRectangle((int)(p.x - W * 0.5f), (int)(p.y - H * 0.5f),
                      (int)W, (int)H, SKYBLUE);
        DrawRectangleLines((int)(p.x - W * 0.5f), (int)(p.y - H * 0.5f),
                           (int)W, (int)H, WHITE);
        // Eyes
        DrawCircle((int)(p.x - 6), (int)(p.y - 6), 4, WHITE);
        DrawCircle((int)(p.x + 6), (int)(p.y - 6), 4, WHITE);
        DrawCircle((int)(p.x - 6), (int)(p.y - 6), 2, BLACK);
        DrawCircle((int)(p.x + 6), (int)(p.y - 6), 2, BLACK);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// GameScene
// ─────────────────────────────────────────────────────────────────────────────
class GameScene : public Node2D
{
public:
    int     m_coins_collected = 0;
    int     m_coins_total     = 0;
    float   m_flash           = 0.0f;   // screen flash on collect

    explicit GameScene() : Node2D("GameScene") {}

    void _ready() override
    {
        Input::map("menu", {KEY_ESCAPE});

        // ── Floor ───────────────────────────────────────────────────────────
        auto* floor_vis = new Node2D("Floor");
        floor_vis->position = Vec2(SW * 0.5f, SH - 60.0f);
        add_child(floor_vis);

        // ── Platforms ───────────────────────────────────────────────────────
        struct PlatDef { float x, y, w; };
        const PlatDef plats[] = {
            {220, 520, 180}, {560, 430, 160}, {900, 340, 200},
            {350, 280, 130}, {750, 210, 150},
        };
        for (auto& p : plats)
        {
            auto* plat = new Node2D(TextFormat("Plat_%.0f", p.x));
            plat->position = Vec2(p.x, p.y);
            add_child(plat);
        }
        (void)plats;

        // ── Coins ────────────────────────────────────────────────────────────
        const Vec2 coin_pos[] = {
            {220, 480}, {560, 390}, {900, 300}, {350, 240},
            {750, 170}, {100, 560}, {1180, 560}, {640, 150},
        };
        m_coins_total = (int)(sizeof(coin_pos) / sizeof(coin_pos[0]));
        for (int i = 0; i < m_coins_total; ++i)
        {
            auto* coin = new Coin(TextFormat("Coin%d", i));
            coin->position = coin_pos[i];
            add_child(coin);
        }

        // ── Player ───────────────────────────────────────────────────────────
        auto* player = new Player("Player");
        player->position   = Vec2(SW * 0.5f, SH - 150.0f);
        player->p_coins    = &m_coins_collected;
        player->m_total    = m_coins_total;
        add_child(player);
    }

    void _update(float dt) override
    {
        Node2D::_update(dt);
        m_flash -= dt;

        if (Input::is_action_just_pressed("menu"))
            g_tree->change_scene_fade<MenuScene>(0.35f, BLACK, 0);

        if (m_coins_collected >= m_coins_total && m_coins_total > 0)
            g_tree->change_scene_fade<WinScene>(0.5f, BLACK, m_coins_total);
    }

    void _draw() override
    {
        ClearBackground(Color{18, 22, 40, 255});

        // Floor
        DrawRectangle(0, SH - 80, SW, 80, Color{40, 80, 40, 255});
        DrawRectangleLines(0, SH - 80, SW, 80, GREEN);

        // Platforms
        const struct { float x, y, w; } plats[] = {
            {220, 520, 180}, {560, 430, 160}, {900, 340, 200},
            {350, 280, 130}, {750, 210, 150},
        };
        for (auto& p : plats)
        {
            DrawRectangle((int)(p.x - p.w * 0.5f), (int)p.y, (int)p.w, 14,
                          Color{60, 110, 60, 255});
            DrawRectangleLines((int)(p.x - p.w * 0.5f), (int)p.y, (int)p.w, 14, GREEN);
        }

        // Flash on collect
        if (m_flash > 0.0f)
            DrawRectangle(0, 0, SW, SH, Color{255, 255, 100, (uint8_t)(m_flash * 120)});

        // HUD
        const std::string hud = "Coins: " + std::to_string(m_coins_collected)
                              + " / " + std::to_string(m_coins_total);
        DrawText(hud.c_str(), 16, 16, 28, GOLD);
        DrawText("ESC = menu   Arrow keys / WASD + SPACE to jump", 16, SH - 30, 18, DARKGRAY);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// WinScene
// ─────────────────────────────────────────────────────────────────────────────
class WinScene : public Node2D
{
public:
    int   m_score  = 0;
    float m_timer  = 0.0f;
    float m_pulse  = 0.0f;

    explicit WinScene(int score) : Node2D("WinScene"), m_score(score) {}

    void _ready() override
    {
        Input::map("confirm", {KEY_ENTER, KEY_SPACE});
    }

    void _update(float dt) override
    {
        Node2D::_update(dt);
        m_timer += dt;
        m_pulse += dt * 3.0f;

        if (m_timer > 1.5f && Input::is_action_just_pressed("confirm"))
            g_tree->change_scene_fade<MenuScene>(0.5f, BLACK, m_score);
    }

    void _draw() override
    {
        ClearBackground(Color{10, 30, 10, 255});

        // Animated stars
        for (int i = 0; i < 60; ++i)
        {
            const float t  = m_timer * 0.4f + i * 0.42f;
            const float x  = fmodf(i * 137.5f + t * 30.0f, (float)SW);
            const float y  = fmodf(i * 97.3f  + t * 18.0f, (float)SH);
            const uint8_t a = (uint8_t)(120 + 100 * sinf(t + i));
            DrawCircle((int)x, (int)y, 2, Color{255, 220, 80, a});
        }

        draw_centered("YOU WIN!", SH * 0.28f, 88, Color{80, 255, 120, 255});

        const std::string sc = "Score: " + std::to_string(m_score) + " coins";
        draw_centered(sc.c_str(), SH * 0.50f, 36, GOLD);

        if (m_timer > 1.5f)
        {
            const uint8_t alpha = (uint8_t)(160 + 95 * sinf(m_pulse));
            draw_centered("PRESS ENTER TO CONTINUE", SH * 0.70f, 28,
                          Color{200, 255, 200, alpha});
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main()
{
    InitWindow(SW, SH, "ZenEngine - Scene Change Demo");
    SetExitKey(KEY_NULL);   // disable ESC closing the window (we use it for menu)
    SetTargetFPS(60);

    GraphLib::Instance().create();

    SceneTree tree;
    g_tree = &tree;

    tree.change_scene<MenuScene>(0);
    tree.start();

    while (!WindowShouldClose())
    {
        Input::update();
        tree.process(GetFrameTime());

        BeginDrawing();
        tree.draw();
        EndDrawing();
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
