#include "pch.hpp"
#include <raylib.h>
#include <cmath>

#include "CharacterBody2D.hpp"
#include "Collider2D.hpp"
#include "CollisionObject2D.hpp"
#include "SceneTree.hpp"
#include "Sprite2D.hpp"
#include "StaticBody2D.hpp"
#include "View2D.hpp"
#include "Assets.hpp"

static constexpr int SCREEN_W = 1280;
static constexpr int SCREEN_H = 720;

// ─── Painted static body that draws its collider ──────────────────────────────
class PaintedBody : public StaticBody2D
{
public:
    Color line_color = YELLOW;
    float line_thick = 3.0f;

    explicit PaintedBody(const std::string& n) : StaticBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;

        if (c->shape == Collider2D::ShapeType::Segment)
        {
            Vec2 a, b;
            c->get_world_segment(a, b);
            DrawLineEx({a.x, a.y}, {b.x, b.y}, line_thick, line_color);

            // Draw small circles at endpoints
            DrawCircleV({a.x, a.y}, 4.0f, line_color);
            DrawCircleV({b.x, b.y}, 4.0f, line_color);

            // Draw angle label
            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float angle = atan2f(dy, dx) * (180.0f / 3.14159265f);
            float mx = (a.x + b.x) * 0.5f;
            float my = (a.y + b.y) * 0.5f - 14.0f;
            DrawText(TextFormat("%.0f deg", angle), (int)mx - 20, (int)my, 16, line_color);
            return;
        }

        std::vector<Vec2> poly;
        c->get_world_polygon(poly);
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2& a = poly[i];
            const Vec2& b = poly[(i + 1) % poly.size()];
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 2.0f, line_color);
        }
    }
};

// ─── Player ───────────────────────────────────────────────────────────────────
class Player : public CharacterBody2D
{
public:
    Vec2 velocity;
    float move_speed = 280.0f;
    float gravity    = 1200.0f;
    float jump_speed = 920.0f;

    explicit Player(const std::string& n) : CharacterBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;

        if (c->shape == Collider2D::ShapeType::Circle)
        {
            Vec2 center = c->get_world_center();
            float r = c->get_world_radius();
            DrawCircleLines((int)center.x, (int)center.y, r, SKYBLUE);
            DrawCircleV({center.x, center.y}, 3.0f, WHITE);
            return;
        }

        std::vector<Vec2> poly;
        c->get_world_polygon(poly);
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2& a = poly[i];
            const Vec2& b = poly[(i + 1) % poly.size()];
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 2.0f, SKYBLUE);
        }
    }

    void _update(float dt) override
    {
        float input = 0.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  input -= 1.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input += 1.0f;

        velocity.x = input * move_speed;
        velocity.y += gravity * dt;

        if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && on_floor)
        {
            velocity.y = -jump_speed;
        }

        move_and_slide(velocity, dt, Vec2(0.0f, -1.0f));

        if (on_floor && velocity.y > 0.0f)
            velocity.y = 0.0f;
    }
};

// ─── Helper: create a segment at position with a given angle and length ──────
static PaintedBody* make_segment(Node* parent, const std::string& name,
                                  float cx, float cy, float angle_deg, float length,
                                  Color color)
{
    float rad = angle_deg * (3.14159265f / 180.0f);
    float hx = cosf(rad) * length * 0.5f;
    float hy = sinf(rad) * length * 0.5f;

    auto* body = new PaintedBody(name);
    body->position = Vec2(cx, cy);
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    body->line_color = color;

    auto* col = new Collider2D("Shape");
    col->shape = Collider2D::ShapeType::Segment;
    col->points = { Vec2(-hx, -hy), Vec2(hx, hy) };
    body->add_child(col);

    parent->add_child(body);
    return body;
}

// ─── Helper: create a rectangle platform ─────────────────────────────────────
static PaintedBody* make_platform(Node* parent, const std::string& name,
                                   float x, float y, float w, float h, Color color)
{
    auto* body = new PaintedBody(name);
    body->position = Vec2(x, y);
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    body->line_color = color;

    auto* col = new Collider2D("Shape");
    col->shape = Collider2D::ShapeType::Rectangle;
    col->size = Vec2(w, h);
    body->add_child(col);

    parent->add_child(body);
    return body;
}

int main()
{
    InitWindow(SCREEN_W, SCREEN_H, "ZenEngine - Segment Collision Test (all angles)");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    SceneTree tree;
    auto* root = new Node2D("Root");

    auto* cam = new View2D("Camera");
    cam->is_current = true;
    root->add_child(cam);

    // ── Floor (rectangle) ────────────────────────────────────────────────────
    make_platform(root, "Floor", -200.0f, 600.0f, 3000.0f, 60.0f, DARKGRAY);

    // ── Segments at every 30° from 0° to 330° arranged in a circle ──────────
    // This tests ALL orientations of segment collision
    const float seg_len = 160.0f;
    const float ring_cx = 640.0f;
    const float ring_cy = 340.0f;
    const float ring_r  = 260.0f;
    const Color angle_colors[] = {
        RED, ORANGE, YELLOW, GREEN, LIME, SKYBLUE,
        BLUE, VIOLET, PURPLE, MAGENTA, PINK, MAROON
    };

    for (int i = 0; i < 12; ++i)
    {
        float angle = i * 30.0f;
        float rad = angle * (3.14159265f / 180.0f);
        float px = ring_cx + cosf(rad) * ring_r;
        float py = ring_cy + sinf(rad) * ring_r;
        make_segment(root, TextFormat("Seg_%d", (int)angle), px, py, angle, seg_len, angle_colors[i]);
    }

    // ── Extra segments: ramps on the floor for slide testing ─────────────────
    make_segment(root, "Ramp_15",   100.0f, 560.0f,  -15.0f, 200.0f, YELLOW);
    make_segment(root, "Ramp_30",   400.0f, 540.0f,  -30.0f, 220.0f, ORANGE);
    make_segment(root, "Ramp_45",   700.0f, 520.0f,  -45.0f, 240.0f, RED);
    make_segment(root, "Ramp_60",  1000.0f, 480.0f,  -60.0f, 260.0f, MAGENTA);
    make_segment(root, "Ramp_75",  1300.0f, 440.0f,  -75.0f, 200.0f, PURPLE);
    make_segment(root, "Wall_90",  1550.0f, 450.0f,  -90.0f, 300.0f, BLUE);

    // ── Player (rectangle collider) ─────────────────────────────────────────
    auto* player = new Player("Player");
    player->position = Vec2(200.0f, 300.0f);
    player->set_collision_layer_bit(0);
    player->set_collision_mask_bit(0);
    root->add_child(player);

    auto* player_col = new Collider2D("Shape");
    player_col->shape = Collider2D::ShapeType::Rectangle;
    player_col->size = Vec2(28.0f, 44.0f);
    player->add_child(player_col);

    // ── Second player (circle collider) for comparison ──────────────────────
    auto* player2 = new Player("PlayerCircle");
    player2->position = Vec2(300.0f, 300.0f);
    player2->set_collision_layer_bit(0);
    player2->set_collision_mask_bit(0);
    player2->move_speed = 0.0f; // controlled separately
    root->add_child(player2);

    auto* player2_col = new Collider2D("Shape");
    player2_col->shape = Collider2D::ShapeType::Circle;
    player2_col->radius = 18.0f;
    player2->add_child(player2_col);

    tree.set_root(root);
    tree.start();
    tree.set_debug_draw_flags(SceneTree::DEBUG_SHAPES | SceneTree::DEBUG_CONTACTS);

    cam->follow_target = player;
    cam->follow_speed = 8.0f;
    cam->position = player->position;

    bool control_circle = false;

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        // TAB switches control between rectangle and circle player
        if (IsKeyPressed(KEY_TAB))
        {
            control_circle = !control_circle;
            cam->follow_target = control_circle ? player2 : player;
        }

        Player* active = control_circle ? player2 : player;
        Player* inactive = control_circle ? player : player2;
        active->move_speed = 280.0f;
        inactive->move_speed = 0.0f;

        tree.process(dt);

        BeginDrawing();
        ClearBackground(Color{20, 24, 36, 255});
        tree.draw();

        // HUD
        DrawText("A/D: move   SPACE: jump   TAB: switch rect/circle player", 16, 14, 20, WHITE);
        DrawText(TextFormat("Active: %s  on_floor[%s]  vel(%.0f, %.0f)",
                            active->name.c_str(),
                            active->on_floor ? "YES" : "NO",
                            active->velocity.x, active->velocity.y),
                 16, 40, 18, SKYBLUE);
        DrawText("Ring: segments every 30 deg | Bottom: ramps 15-90 deg", 16, 64, 18, GRAY);

        // Draw segment angle legend
        for (int i = 0; i < 12; ++i)
        {
            DrawRectangle(SCREEN_W - 160, 10 + i * 18, 12, 12, angle_colors[i]);
            DrawText(TextFormat("%d deg", i * 30), SCREEN_W - 142, 8 + i * 18, 16, angle_colors[i]);
        }

        EndDrawing();
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
