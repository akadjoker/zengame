#include "pch.hpp"
#include <raylib.h>

#include "CharacterBody2D.hpp"
#include "Collider2D.hpp"
#include "SceneTree.hpp"
#include "StaticBody2D.hpp"
#include "TextNode2D.hpp"
#include "View2D.hpp"

static constexpr int SCREEN_W = 1400;
static constexpr int SCREEN_H = 820;

class DebugStaticShape : public StaticBody2D
{
public:
    Color fill = Color{120, 70, 42, 180};
    Color line = Color{255, 220, 140, 255};

    explicit DebugStaticShape(const std::string& n) : StaticBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;

        if (c->shape == Collider2D::ShapeType::Segment)
        {
            Vec2 a, b;
            c->get_world_segment(a, b);
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 4.0f, line);
            return;
        }

        std::vector<Vec2> poly;
        c->get_world_polygon(poly);
        if (poly.size() == 3)
        {
            DrawTriangle({poly[0].x, poly[0].y},
                         {poly[1].x, poly[1].y},
                         {poly[2].x, poly[2].y},
                         fill);
        }
        else if (poly.size() == 4)
        {
            DrawTriangle({poly[0].x, poly[0].y},
                         {poly[1].x, poly[1].y},
                         {poly[2].x, poly[2].y},
                         fill);
            DrawTriangle({poly[0].x, poly[0].y},
                         {poly[2].x, poly[2].y},
                         {poly[3].x, poly[3].y},
                         fill);
        }

        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2& a = poly[i];
            const Vec2& b = poly[(i + 1) % poly.size()];
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 2.0f, line);
        }
    }
};

class ShapePlayer : public CharacterBody2D
{
public:
    Vec2 velocity = Vec2();
    float input_x = 0.0f;
    bool jump_requested = false;
    float move_speed = 280.0f;
    float gravity = 1200.0f;
    float jump_speed = 860.0f;

    explicit ShapePlayer(const std::string& n) : CharacterBody2D(n) {}

    void respawn(const Vec2& pos)
    {
        position = pos;
        velocity = Vec2();
    }

    void _update(float dt) override
    {
        velocity.x = input_x * move_speed;
        velocity.y += gravity * dt;

        if (jump_requested && on_floor)
        {
            velocity.y = -jump_speed;
        }
        jump_requested = false;

        move_and_slide(velocity, dt, Vec2(0.0f, -1.0f));

        if (on_floor && velocity.y > 0.0f)
        {
            velocity.y = 0.0f;
        }
    }

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;

        std::vector<Vec2> poly;
        c->get_world_polygon(poly);
        if (poly.size() == 4)
        {
            DrawTriangle({poly[0].x, poly[0].y},
                         {poly[1].x, poly[1].y},
                         {poly[2].x, poly[2].y},
                         Color{110, 190, 255, 80});
            DrawTriangle({poly[0].x, poly[0].y},
                         {poly[2].x, poly[2].y},
                         {poly[3].x, poly[3].y},
                         Color{110, 190, 255, 80});
        }

        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2& a = poly[i];
            const Vec2& b = poly[(i + 1) % poly.size()];
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 2.0f, SKYBLUE);
        }

        const Vec2 p = get_global_position();
        DrawCircleV({p.x, p.y}, 3.0f, YELLOW);
        DrawLineEx({p.x, p.y}, {p.x + velocity.x * 0.15f, p.y + velocity.y * 0.15f}, 2.0f, MAGENTA);

        if (on_floor)
        {
            DrawLineEx({p.x, p.y}, {p.x + floor_normal.x * 28.0f, p.y + floor_normal.y * 28.0f}, 2.0f, GREEN);
        }
    }
};

static DebugStaticShape* make_rect(Node* parent, const std::string& name,
                                   float x, float y, float w, float h)
{
    auto* body = new DebugStaticShape(name);
    body->position = Vec2(x, y);
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);

    auto* col = new Collider2D("Shape");
    col->shape = Collider2D::ShapeType::Rectangle;
    col->size = Vec2(w, h);
    body->add_child(col);

    parent->add_child(body);
    return body;
}

static DebugStaticShape* make_poly(Node* parent, const std::string& name,
                                   float x, float y, const std::vector<Vec2>& pts)
{
    auto* body = new DebugStaticShape(name);
    body->position = Vec2(x, y);
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);

    auto* col = new Collider2D("Shape");
    col->shape = Collider2D::ShapeType::Polygon;
    col->points = pts;
    body->add_child(col);

    parent->add_child(body);
    return body;
}

static DebugStaticShape* make_segment(Node* parent, const std::string& name,
                                      float x, float y, const Vec2& a, const Vec2& b)
{
    auto* body = new DebugStaticShape(name);
    body->position = Vec2(x, y);
    body->line = Color{200, 255, 160, 255};
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);

    auto* col = new Collider2D("Shape");
    col->shape = Collider2D::ShapeType::Segment;
    col->points = {a, b};
    body->add_child(col);

    parent->add_child(body);
    return body;
}

int main()
{
    InitWindow(SCREEN_W, SCREEN_H, "ZenEngine - CharacterBody2D Shape Debug");
    SetTargetFPS(60);

    SceneTree tree;
    Node2D* root = new Node2D("Root");

    auto* cam = new View2D("Camera");
    cam->is_current = true;
    root->add_child(cam);

    make_rect(root, "Floor", 0.0f, 700.0f, 2400.0f, 80.0f);

    // Zone 1: rectangles
    make_rect(root, "RectBoxA", 120.0f, 620.0f, 120.0f, 80.0f);
    make_rect(root, "RectBoxB", 330.0f, 540.0f, 140.0f, 24.0f);
    make_rect(root, "RectWall", 540.0f, 480.0f, 30.0f, 220.0f);

    // Zone 2: polygons
    make_poly(root, "PolyRampA", 760.0f, 700.0f, {
        Vec2(0.0f, 0.0f),
        Vec2(220.0f, -120.0f),
        Vec2(220.0f, 0.0f)
    });
    make_poly(root, "PolySlopeB", 1100.0f, 700.0f, {
        Vec2(0.0f, 0.0f),
        Vec2(170.0f, -60.0f),
        Vec2(210.0f, -10.0f),
        Vec2(210.0f, 0.0f)
    });
    make_poly(root, "PolyBlock", 980.0f, 510.0f, {
        Vec2(-60.0f,  40.0f),
        Vec2(  0.0f, -50.0f),
        Vec2( 55.0f,  10.0f),
        Vec2( 10.0f,  55.0f)
    });

    // Zone 3: segments (expected to be the weakest case with current solver)
    make_segment(root, "SegmentRampA", 1460.0f, 700.0f, Vec2(0.0f, 0.0f), Vec2(180.0f, -90.0f));
    make_segment(root, "SegmentRampB", 1720.0f, 640.0f, Vec2(0.0f, 0.0f), Vec2(180.0f, -30.0f));
    make_segment(root, "SegmentWall", 1940.0f, 700.0f, Vec2(0.0f, 0.0f), Vec2(0.0f, -180.0f));
    make_poly(root, "SegmentRampAProxy", 1460.0f, 700.0f, {
        Vec2(0.0f, 0.0f),
        Vec2(180.0f, -90.0f),
        Vec2(180.0f, -74.0f),
        Vec2(0.0f, 16.0f)
    })->visible = false;
    make_poly(root, "SegmentRampBProxy", 1720.0f, 640.0f, {
        Vec2(0.0f, 0.0f),
        Vec2(180.0f, -30.0f),
        Vec2(180.0f, -14.0f),
        Vec2(0.0f, 16.0f)
    })->visible = false;

    auto* player = new ShapePlayer("Player");
    player->position = Vec2(70.0f, 640.0f);
    player->set_collision_layer_bit(0);
    player->set_collision_mask_bit(0);
    root->add_child(player);

    auto* player_col = new Collider2D("Shape");
    player_col->shape = Collider2D::ShapeType::Rectangle;
    player_col->size = Vec2(32.0f, 48.0f);
    player->add_child(player_col);

    auto* hud = new TextNode2D("HUD");
    hud->font_size = 18.0f;
    hud->position = Vec2(16.0f, 16.0f);

    tree.set_root(root);
    tree.set_ui_root(hud);
    tree.start();
    tree.set_debug_draw_flags(SceneTree::DEBUG_CONTACTS | SceneTree::DEBUG_NAMES);

    cam->follow_target = player;
    cam->follow_speed = 8.0f;
    cam->position = player->position + Vec2(220.0f, -120.0f);

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        float input = 0.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  input -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) input += 1.0f;
        player->input_x = input;

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
        {
            player->jump_requested = true;
        }

        if (IsKeyPressed(KEY_ONE))   player->respawn(Vec2(70.0f, 640.0f));
        if (IsKeyPressed(KEY_TWO))   player->respawn(Vec2(820.0f, 520.0f));
        if (IsKeyPressed(KEY_THREE)) player->respawn(Vec2(1490.0f, 560.0f));
        if (IsKeyPressed(KEY_R))     player->respawn(Vec2(70.0f, 640.0f));

        if (IsKeyPressed(KEY_F1))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_CONTACTS);
        }
        if (IsKeyPressed(KEY_F2))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_NAMES);
        }

        tree.process(dt);

        hud->text =
            "A/D move  SPACE jump  R reset  1/2/3 teleport zones\n"
            "F1 contacts  F2 names\n"
            "Zone1 Rectangles  Zone2 Polygons  Zone3 Segments (known weak case)\n"
            "on_floor[" + std::string(player->on_floor ? "YES" : "NO") + "]"
            + " vel(" + std::to_string((int)player->velocity.x) + ", " + std::to_string((int)player->velocity.y) + ")";

        BeginDrawing();
        ClearBackground(Color{18, 18, 24, 255});
        tree.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
