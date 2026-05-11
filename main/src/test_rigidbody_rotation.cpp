#include "pch.hpp"
#include <raylib.h>
#include <cmath>

#include "Assets.hpp"
#include "RigidBody2D.hpp"
#include "Collider2D.hpp"
#include "CollisionObject2D.hpp"
#include "SceneTree.hpp"
#include "StaticBody2D.hpp"
#include "View2D.hpp"

static constexpr int SCREEN_W = 1280;
static constexpr int SCREEN_H = 720;

// ─── Visual RigidBody ─────────────────────────────────────────────────────────
class VisualRigidBody : public RigidBody2D
{
public:
    Color color = WHITE;

    explicit VisualRigidBody(const std::string& n) : RigidBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;

        if (c->shape == Collider2D::ShapeType::Circle)
        {
            const Vec2 center = c->get_world_center();
            const float r = c->get_world_radius();
            DrawCircleV({center.x, center.y}, r, ColorAlpha(color, 0.6f));
            DrawCircleLines((int)center.x, (int)center.y, r, color);
            // Draw rotation indicator line
            float rad = rotation * (3.14159265f / 180.0f);
            DrawLineEx({center.x, center.y},
                       {center.x + cosf(rad) * r, center.y + sinf(rad) * r},
                       2.0f, WHITE);
            return;
        }

        std::vector<Vec2> poly;
        c->get_world_polygon(poly);
        if (poly.size() >= 3)
        {
            for (size_t i = 1; i < poly.size() - 1; ++i)
            {
                DrawTriangle({poly[0].x, poly[0].y},
                             {poly[i].x, poly[i].y},
                             {poly[i+1].x, poly[i+1].y},
                             ColorAlpha(color, 0.5f));
            }
        }
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2& a = poly[i];
            const Vec2& b = poly[(i + 1) % poly.size()];
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 2.0f, color);
        }
        // Rotation indicator: line from center toward local +X
        const Vec2 center = c->get_world_center();
        float rad = rotation * (3.14159265f / 180.0f);
        DrawLineEx({center.x, center.y},
                   {center.x + cosf(rad) * 20.0f, center.y + sinf(rad) * 20.0f},
                   2.0f, WHITE);
    }
};

// ─── Static walls ─────────────────────────────────────────────────────────────
class WallBody : public StaticBody2D
{
public:
    Color color = DARKGRAY;
    explicit WallBody(const std::string& n) : StaticBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;

        if (c->shape == Collider2D::ShapeType::Segment)
        {
            Vec2 a, b;
            c->get_world_segment(a, b);
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 4.0f, color);
            return;
        }

        std::vector<Vec2> poly;
        c->get_world_polygon(poly);
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2& a = poly[i];
            const Vec2& b = poly[(i + 1) % poly.size()];
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 3.0f, color);
        }
    }
};

static WallBody* make_wall(Node* parent, const char* name, float x, float y, float w, float h, Color col)
{
    auto* body = new WallBody(name);
    body->position = Vec2(x, y);
    body->color = col;
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    auto* c = new Collider2D("Shape");
    c->shape = Collider2D::ShapeType::Rectangle;
    c->size = Vec2(w, h);
    body->add_child(c);
    parent->add_child(body);
    return body;
}

static WallBody* make_segment_wall(Node* parent, const char* name,
                                    float cx, float cy, float angle_deg, float len, Color col)
{
    float rad = angle_deg * (3.14159265f / 180.0f);
    float hx = cosf(rad) * len * 0.5f;
    float hy = sinf(rad) * len * 0.5f;

    auto* body = new WallBody(name);
    body->position = Vec2(cx, cy);
    body->color = col;
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    auto* c = new Collider2D("Shape");
    c->shape = Collider2D::ShapeType::Segment;
    c->points = { Vec2(-hx, -hy), Vec2(hx, hy) };
    body->add_child(c);
    parent->add_child(body);
    return body;
}

static VisualRigidBody* spawn_box(Node* parent, float x, float y, float w, float h,
                                   Color col, float ang_vel = 0.0f)
{
    auto* body = new VisualRigidBody(TextFormat("Box_%.0f_%.0f", x, y));
    body->position = Vec2(x, y);
    body->color = col;
    body->bounciness = 0.4f;
    body->friction = 0.3f;
    //body->angular_velocity = ang_vel;
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    auto* c = new Collider2D("Shape");
    c->shape = Collider2D::ShapeType::Rectangle;
    c->size = Vec2(w, h);
    body->add_child(c);
    parent->add_child(body);
    return body;
}

static VisualRigidBody* spawn_circle(Node* parent, float x, float y, float r,
                                      Color col, float ang_vel = 0.0f)
{
    auto* body = new VisualRigidBody(TextFormat("Circle_%.0f_%.0f", x, y));
    body->position = Vec2(x, y);
    body->color = col;
    body->bounciness = 0.5f;
    body->friction = 0.2f;
   // body->angular_velocity = ang_vel;
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    auto* c = new Collider2D("Shape");
    c->shape = Collider2D::ShapeType::Circle;
    c->radius = r;
    body->add_child(c);
    parent->add_child(body);
    return body;
}

int main()
{
    InitWindow(SCREEN_W, SCREEN_H, "ZenEngine - RigidBody2D Rotation Test");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    SceneTree tree;
    auto* root = new Node2D("Root");

    auto* cam = new View2D("Camera");
    cam->is_current = true;
    cam->position = Vec2(640.0f, 360.0f);
    root->add_child(cam);

    // ── Static world ─────────────────────────────────────────────────────────
    make_wall(root, "Floor",      -100.0f, 650.0f, 1500.0f, 40.0f, DARKGRAY);
    make_wall(root, "WallLeft",   -100.0f, 200.0f, 40.0f, 500.0f, DARKGRAY);
    make_wall(root, "WallRight",  1360.0f, 200.0f, 40.0f, 500.0f, DARKGRAY);

    // Angled ramps
    make_segment_wall(root, "Ramp1", 300.0f, 580.0f, -25.0f, 300.0f, YELLOW);
    make_segment_wall(root, "Ramp2", 800.0f, 550.0f, -40.0f, 350.0f, ORANGE);
    make_segment_wall(root, "Ramp3", 1100.0f, 500.0f, 20.0f, 250.0f, GREEN);

    // A ledge
    make_wall(root, "Ledge", 500.0f, 420.0f, 200.0f, 20.0f, Color{100, 160, 200, 255});

    // ── Dynamic bodies — boxes with different shapes ─────────────────────────
    // Boxes dropped at various positions — should spin on impact with ramps
    spawn_box(root, 250.0f, 100.0f, 40.0f, 40.0f, RED, 45.0f);
    spawn_box(root, 350.0f, 50.0f,  50.0f, 30.0f, ORANGE, -30.0f);
    spawn_box(root, 500.0f, 80.0f,  35.0f, 60.0f, YELLOW, 0.0f);
    spawn_box(root, 700.0f, 30.0f,  44.0f, 44.0f, GREEN, 90.0f);
    spawn_box(root, 900.0f, 60.0f,  30.0f, 50.0f, SKYBLUE, -60.0f);
    spawn_box(root, 1050.0f, 40.0f, 48.0f, 28.0f, BLUE, 120.0f);

    // Circles
    spawn_circle(root, 200.0f, 150.0f, 20.0f, MAGENTA, 200.0f);
    spawn_circle(root, 600.0f, 100.0f, 25.0f, PURPLE, -150.0f);
    spawn_circle(root, 850.0f, 50.0f,  18.0f, PINK, 300.0f);
    spawn_circle(root, 1100.0f, 80.0f, 22.0f, VIOLET, 0.0f);

    tree.set_root(root);
    tree.start();
    tree.set_debug_draw_flags(SceneTree::DEBUG_SHAPES | SceneTree::DEBUG_CONTACTS);

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        // Click to spawn a new box at mouse position
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mp = GetMousePosition();
            Vec2 world_pos = Vec2(mp.x + cam->position.x - SCREEN_W * 0.5f,
                                  mp.y + cam->position.y - SCREEN_H * 0.5f);
            spawn_box(root, world_pos.x, world_pos.y, 30.0f + GetRandomValue(0, 30),
                      25.0f + GetRandomValue(0, 25),
                      Color{(unsigned char)GetRandomValue(100, 255),
                            (unsigned char)GetRandomValue(100, 255),
                            (unsigned char)GetRandomValue(100, 255), 255},
                      (float)GetRandomValue(-200, 200));
        }

        // Right-click to spawn circle
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            Vector2 mp = GetMousePosition();
            Vec2 world_pos = Vec2(mp.x + cam->position.x - SCREEN_W * 0.5f,
                                  mp.y + cam->position.y - SCREEN_H * 0.5f);
            spawn_circle(root, world_pos.x, world_pos.y, 12.0f + GetRandomValue(0, 20),
                         Color{(unsigned char)GetRandomValue(100, 255),
                               (unsigned char)GetRandomValue(100, 255),
                               (unsigned char)GetRandomValue(100, 255), 255},
                         (float)GetRandomValue(-300, 300));
        }

        tree.process(dt);

        BeginDrawing();
        ClearBackground(Color{16, 20, 30, 255});
        tree.draw();

        DrawText("LMB: spawn box   RMB: spawn circle   Bodies spin on impact!", 16, 14, 20, WHITE);
        DrawText(TextFormat("Bodies: %d", (int)root->get_child_count()), 16, 40, 18, SKYBLUE);
        DrawText("White line = rotation indicator", 16, 62, 18, GRAY);

        EndDrawing();
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
