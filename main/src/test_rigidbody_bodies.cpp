#include "pch.hpp"
#include <raylib.h>
#include <cmath>
#include <string>

#include "Assets.hpp"
#include "RigidBody2D.hpp"
#include "Collider2D.hpp"
#include "CollisionObject2D.hpp"
#include "SceneTree.hpp"
#include "StaticBody2D.hpp"
#include "Area2D.hpp"
#include "TextNode2D.hpp"
#include "View2D.hpp"

static constexpr int SCREEN_W = 1280;
static constexpr int SCREEN_H = 720;

// ─── Visual helpers ───────────────────────────────────────────────────────────

class VisualBody : public RigidBody2D
{
public:
    Color color = WHITE;
    explicit VisualBody(const std::string& n) : RigidBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;

        if (c->shape == Collider2D::ShapeType::Circle)
        {
            const Vec2 center = c->get_world_center();
            const float r = c->get_world_radius();
            DrawCircleV({center.x, center.y}, r, ColorAlpha(color, 0.65f));
            DrawCircleLines((int)center.x, (int)center.y, r, color);
            // rotation indicator
            const float rad = rotation * (3.14159265f / 180.0f);
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
                DrawTriangle({poly[0].x, poly[0].y},
                             {poly[i].x, poly[i].y},
                             {poly[i + 1].x, poly[i + 1].y},
                             ColorAlpha(color, 0.5f));
        }
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2& a = poly[i];
            const Vec2& b = poly[(i + 1) % poly.size()];
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 2.0f, color);
        }
        const Vec2 center = c->get_world_center();
        const float rad = rotation * (3.14159265f / 180.0f);
        DrawLineEx({center.x, center.y},
                   {center.x + cosf(rad) * 18.0f, center.y + sinf(rad) * 18.0f},
                   2.0f, WHITE);
    }
};

class WallBody : public StaticBody2D
{
public:
    Color color = DARKGRAY;
    explicit WallBody(const std::string& n) : StaticBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;
        std::vector<Vec2> poly;
        c->get_world_polygon(poly);
        if (poly.size() >= 3)
        {
            for (size_t i = 1; i < poly.size() - 1; ++i)
                DrawTriangle({poly[0].x, poly[0].y},
                             {poly[i].x, poly[i].y},
                             {poly[i + 1].x, poly[i + 1].y},
                             ColorAlpha(color, 0.4f));
        }
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2& a = poly[i];
            const Vec2& b = poly[(i + 1) % poly.size()];
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 2.5f, color);
        }
    }
};

// ─── Factory helpers ──────────────────────────────────────────────────────────

static WallBody* make_wall(Node* parent, const char* name,
                            float x, float y, float w, float h, Color col = DARKGRAY)
{
    auto* body = new WallBody(name);
    body->position = Vec2(x, y);
    body->color    = col;
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    auto* c = new Collider2D("Shape");
    c->shape = Collider2D::ShapeType::Rectangle;
    c->size  = Vec2(w, h);
    body->add_child(c);
    parent->add_child(body);
    return body;
}

static VisualBody* make_box(Node* parent, const char* name,
                             float x, float y, float w, float h, Color col,
                             float mass = 1.0f, float bounciness = 0.3f, float friction = 0.3f)
{
    auto* body = new VisualBody(name);
    body->position   = Vec2(x, y);
    body->color      = col;
    body->mass       = mass;
    body->bounciness = bounciness;
    body->friction   = friction;
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    auto* c = new Collider2D("Shape");
    c->shape = Collider2D::ShapeType::Rectangle;
    c->size  = Vec2(w, h);
    body->add_child(c);
    parent->add_child(body);
    return body;
}

static VisualBody* make_circle(Node* parent, const char* name,
                                float x, float y, float r, Color col,
                                float mass = 1.0f, float bounciness = 0.5f, float friction = 0.2f)
{
    auto* body = new VisualBody(name);
    body->position   = Vec2(x, y);
    body->color      = col;
    body->mass       = mass;
    body->bounciness = bounciness;
    body->friction   = friction;
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    auto* c = new Collider2D("Shape");
    c->shape  = Collider2D::ShapeType::Circle;
    c->radius = r;
    body->add_child(c);
    parent->add_child(body);
    return body;
}

// ─── Scene label ─────────────────────────────────────────────────────────────

static void add_label(Node* parent, float x, float y, const char* text, Color col = LIGHTGRAY)
{
    auto* t  = new TextNode2D(text);
    t->position  = Vec2(x, y);
    t->font_size = 18;
    t->color     = col;
    t->text      = text;
    parent->add_child(t);
}

// ─── Scene builder ────────────────────────────────────────────────────────────
//
// The screen is divided into sections (left / right):
//
//  LEFT COLUMN (x ≈ 0..620)
//  [A] Head-on collision — equal masses, circles
//  [B] Oblique hit — box hits resting box at angle
//  [C] Domino chain
//
//  RIGHT COLUMN (x ≈ 660..1280)
//  [D] Heavy ball hits light cluster
//  [E] Stack of boxes (stacking test)
//  [F] Area2D trigger zone (floor trigger lights up when entered)
//
// A shared floor + side walls bound the whole arena.

static void build_scene(Node* root)
{
    // ── Boundary walls ────────────────────────────────────────────────────────
    make_wall(root, "Floor",     -40.0f,  700.0f, 1380.0f,  40.0f);
    make_wall(root, "WallLeft",  -40.0f, -200.0f,   40.0f, 1000.0f);
    make_wall(root, "WallRight",1280.0f, -200.0f,   40.0f, 1000.0f);

    // Centre divider (just visual / structural)
    make_wall(root, "Divider",   618.0f,  100.0f,   12.0f,  560.0f, Color{50,50,60,255});

    // ── [A] Head-on – equal mass circles ─────────────────────────────────────
    add_label(root,  40.0f,  20.0f, "A: head-on (equal mass)", YELLOW);
    {
        auto* a = make_circle(root, "A_Left",  120.0f, 120.0f, 22.0f, RED,   1.0f, 0.6f, 0.1f);
        auto* b = make_circle(root, "A_Right", 430.0f, 120.0f, 22.0f, BLUE,  1.0f, 0.6f, 0.1f);
        a->linear_velocity = Vec2( 300.0f, 0.0f);
        b->linear_velocity = Vec2(-300.0f, 0.0f);
        a->gravity_scale = 0.0f;
        b->gravity_scale = 0.0f;
        a->linear_damping = 0.05f;
        b->linear_damping = 0.05f;
    }

    // ── [B] Oblique box hit ───────────────────────────────────────────────────
    add_label(root,  40.0f, 200.0f, "B: oblique box hit", YELLOW);
    {
        // Resting target box
        auto* target = make_box(root, "B_Target", 350.0f, 330.0f, 50.0f, 50.0f, GREEN, 1.0f, 0.3f, 0.4f);
        target->gravity_scale = 0.0f;
        // Projectile coming diagonally
        auto* proj   = make_box(root, "B_Proj",   120.0f, 230.0f, 40.0f, 40.0f, ORANGE, 1.0f, 0.3f, 0.4f);
        proj->linear_velocity = Vec2(280.0f, 140.0f);
        proj->gravity_scale   = 0.0f;
        proj->rotation        = 15.0f;
    }

    // ── [C] Domino chain ──────────────────────────────────────────────────────
    add_label(root,  40.0f, 400.0f, "C: domino chain", YELLOW);
    {
        // Trigger ball
        auto* ball = make_circle(root, "C_Ball", 50.0f, 490.0f, 16.0f, RED, 1.0f, 0.1f, 0.5f);
        ball->linear_velocity = Vec2(220.0f, 0.0f);
        ball->gravity_scale   = 0.0f;

        // Row of standing dominoes
        const float base_y = 510.0f;
        for (int i = 0; i < 8; ++i)
        {
            char name[32];
            snprintf(name, sizeof(name), "C_Dom%d", i);
            auto* dom = make_box(root, name,
                                 140.0f + i * 60.0f, base_y, 12.0f, 48.0f,
                                 Color{180, 140, 100, 255}, 1.0f, 0.2f, 0.6f);
            dom->gravity_scale = 0.0f;
        }
    }

    // ── [D] Heavy ball hits light cluster ────────────────────────────────────
    add_label(root, 680.0f,  20.0f, "D: heavy vs light", YELLOW);
    {
        // Heavy ball
        auto* heavy = make_circle(root, "D_Heavy", 700.0f, 120.0f, 28.0f, ORANGE, 5.0f, 0.4f, 0.2f);
        heavy->linear_velocity = Vec2(250.0f, 0.0f);
        heavy->gravity_scale   = 0.0f;

        // Cluster of light circles
        const float cx = 1050.0f, cy = 120.0f, cr = 18.0f;
        const Vec2 offsets[] = {
            {0,0}, {-cr*2.1f, 0}, {cr*2.1f, 0},
            {-cr, -cr*1.8f}, {cr, -cr*1.8f}
        };
        for (int i = 0; i < 5; ++i)
        {
            char name[32]; snprintf(name, sizeof(name), "D_Light%d", i);
            auto* light = make_circle(root, name,
                                      cx + offsets[i].x, cy + offsets[i].y,
                                      cr, SKYBLUE, 0.5f, 0.4f, 0.2f);
            light->gravity_scale = 0.0f;
        }
    }

    // ── [E] Stacking test ────────────────────────────────────────────────────
    add_label(root, 680.0f, 270.0f, "E: box stack", YELLOW);
    {
        // Platform
        make_wall(root, "E_Platform", 750.0f, 540.0f, 300.0f, 20.0f, Color{80,100,80,255});

        const float sx = 900.0f;
        const float box_h = 36.0f;
        const Color cols[] = {RED, ORANGE, YELLOW, GREEN, SKYBLUE};
        for (int i = 0; i < 5; ++i)
        {
            char name[32]; snprintf(name, sizeof(name), "E_Box%d", i);
            make_box(root, name,
                     sx, 510.0f - i * (box_h + 2.0f),
                     60.0f, box_h, cols[i % 5],
                     1.0f, 0.1f, 0.5f);
        }

        // A pusher box dropped from the side
        auto* pusher = make_box(root, "E_Pusher", 690.0f, 390.0f, 50.0f, 50.0f, PURPLE, 2.0f, 0.2f, 0.4f);
        pusher->linear_velocity = Vec2(160.0f, 0.0f);
    }

    // ── [F] Area2D trigger zone ───────────────────────────────────────────────
    add_label(root, 680.0f, 590.0f, "F: Area2D trigger (turns magenta)", YELLOW);
    {
        // The zone
        auto* zone = new Area2D("F_Zone");
        zone->position = Vec2(900.0f, 660.0f);
        zone->set_collision_layer_bit(0);
        zone->set_collision_mask_bit(0);
        auto* zc = new Collider2D("Shape");
        zc->shape = Collider2D::ShapeType::Rectangle;
        zc->size  = Vec2(200.0f, 40.0f);
        zone->add_child(zc);
        root->add_child(zone);

        // Sensor indicator (drawn as a semi-transparent rectangle)
        // We attach a child TextNode2D that changes color on enter/exit
        auto* lbl = new TextNode2D("zone_lbl");
        lbl->position  = Vec2(900.0f, 655.0f);
        lbl->font_size = 16;
        lbl->color     = Color{100,100,200,200};
        lbl->text      = "[zone]";
        root->add_child(lbl);

        zone->on_body_entered = [lbl](CollisionObject2D* /*b*/)
        {
            lbl->color = MAGENTA;
            lbl->text  = "[zone: ENTERED]";
        };
        zone->on_body_exited = [lbl](CollisionObject2D* /*b*/)
        {
            lbl->color = Color{100,100,200,200};
            lbl->text  = "[zone]";
        };
    }
}

// ─── Draw zone outlines ───────────────────────────────────────────────────────

static void draw_zone_overlay()
{
    // Area [F] outline
    DrawRectangleLinesEx({800.0f, 640.0f, 200.0f, 40.0f}, 2.0f,
                          Color{100, 100, 255, 180});
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main()
{
    InitWindow(SCREEN_W, SCREEN_H, "ZenEngine - RigidBody2D Body-vs-Body Test");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    SceneTree tree;
    auto* root = new Node2D("Root");

    auto* cam = new View2D("Camera");
    cam->is_current = true;
    cam->position   = Vec2(640.0f, 360.0f);
    root->add_child(cam);

    build_scene(root);

    tree.set_root(root);
    tree.start();
    tree.set_debug_draw_flags(SceneTree::DEBUG_SHAPES | SceneTree::DEBUG_CONTACTS);

    bool paused = false;

    while (!WindowShouldClose())
    {
        const float dt = paused ? 0.0f : GetFrameTime();

        if (IsKeyPressed(KEY_SPACE))  paused = !paused;
        if (IsKeyPressed(KEY_R))
        {
            // Rebuild scene
            tree.set_root(nullptr);
            auto* new_root = new Node2D("Root");
            auto* new_cam  = new View2D("Camera");
            new_cam->is_current = true;
            new_cam->position   = Vec2(640.0f, 360.0f);
            new_root->add_child(new_cam);
            build_scene(new_root);
            tree.set_root(new_root);
            tree.start();
            root = new_root;
            paused = false;
        }

        // Click to spawn a random box at mouse
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mp = GetMousePosition();
            Vec2 wp    = Vec2(mp.x + cam->position.x - SCREEN_W * 0.5f,
                              mp.y + cam->position.y - SCREEN_H * 0.5f);
            char name[32];
            snprintf(name, sizeof(name), "Spawn_%d", GetRandomValue(0, 9999));
            auto* b = make_box(root, name, wp.x, wp.y,
                               20.0f + GetRandomValue(0, 40),
                               20.0f + GetRandomValue(0, 40),
                               Color{(uint8_t)GetRandomValue(120,255),
                                     (uint8_t)GetRandomValue(120,255),
                                     (uint8_t)GetRandomValue(120,255), 255},
                               0.5f + GetRandomValue(0, 30) * 0.1f, 0.3f, 0.4f);
            b->linear_velocity = Vec2((float)GetRandomValue(-150, 150), -200.0f);
        }

        // Right-click to spawn a circle
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            Vector2 mp = GetMousePosition();
            Vec2 wp    = Vec2(mp.x + cam->position.x - SCREEN_W * 0.5f,
                              mp.y + cam->position.y - SCREEN_H * 0.5f);
            char name[32];
            snprintf(name, sizeof(name), "SpawnC_%d", GetRandomValue(0, 9999));
            auto* b = make_circle(root, name, wp.x, wp.y,
                                  10.0f + GetRandomValue(0, 20),
                                  Color{(uint8_t)GetRandomValue(120,255),
                                        (uint8_t)GetRandomValue(120,255),
                                        (uint8_t)GetRandomValue(120,255), 255},
                                  0.5f + GetRandomValue(0, 20) * 0.1f, 0.5f, 0.2f);
            b->linear_velocity = Vec2((float)GetRandomValue(-200, 200), -300.0f);
        }

        tree.process(dt);

        BeginDrawing();
        ClearBackground(Color{16, 20, 30, 255});
        tree.draw();
        draw_zone_overlay();

        // HUD
        DrawText("SPACE: pause   R: reset   LMB: spawn box   RMB: spawn circle", 8, 8, 18, WHITE);
        DrawText(paused ? "[ PAUSED ]" : "", 600, 8, 20, YELLOW);

        EndDrawing();
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
