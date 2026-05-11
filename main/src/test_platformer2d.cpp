#include "pch.hpp"
#include <raylib.h>

#include "Assets.hpp"
#include "CharacterBody2D.hpp"
#include "Collider2D.hpp"
#include "SceneTree.hpp"
#include "Sprite2D.hpp"
#include "StaticBody2D.hpp"
#include "View2D.hpp"

static constexpr int SCREEN_W = 1280;
static constexpr int SCREEN_H = 720;

static std::string make_platformer_bg_image()
{
    const std::string path = "/tmp/zengame_platformer_bg.png";
    Image img = GenImageColor(2400, 900, Color{110, 186, 255, 255});

    for (int y = 0; y < img.height; ++y)
    {
        const float t = (float)y / (float)(img.height - 1);
        const Color c = {
            (unsigned char)(110 - t * 48.0f),
            (unsigned char)(186 - t * 74.0f),
            (unsigned char)(255 - t * 84.0f),
            255
        };
        ImageDrawLine(&img, 0, y, img.width - 1, y, c);
    }

    for (int i = 0; i < 24; ++i)
    {
        const int x = 70 + i * 95;
        const int y = 90 + (i % 5) * 36;
        ImageDrawCircle(&img, x, y, 34, Color{255, 255, 255, 185});
        ImageDrawCircle(&img, x + 28, y + 6, 28, Color{255, 255, 255, 185});
        ImageDrawCircle(&img, x - 24, y + 10, 24, Color{255, 255, 255, 185});
    }

    for (int x = -40; x < img.width + 80; x += 180)
    {
        const int top = 600 + ((x + 40) / 180 % 4) * 22;
        ImageDrawCircle(&img, x + 90, top - 110, 110, Color{92, 138, 202, 255});
        ImageDrawRectangle(&img, x, top - 110, 180, 250, Color{92, 138, 202, 255});
    }

    ExportImage(img, path.c_str());
    UnloadImage(img);
    return path;
}

static std::string make_platform_tile_image()
{
    const std::string path = "/tmp/zengame_platform_tile.png";
    Image img = GenImageColor(96, 32, Color{110, 78, 45, 255});

    ImageDrawRectangle(&img, 0, 0, img.width, 8, Color{90, 168, 78, 255});
    ImageDrawRectangle(&img, 0, 8, img.width, 4, Color{124, 198, 96, 255});

    for (int x = 0; x < img.width; x += 12)
    {
        ImageDrawRectangle(&img, x, 14 + (x / 12 % 2), 8, 6, Color{124, 90, 54, 255});
    }

    for (int x = 6; x < img.width; x += 18)
    {
        ImageDrawCircle(&img, x, 24, 2, Color{86, 58, 32, 255});
        ImageDrawCircle(&img, x + 7, 20, 2, Color{96, 66, 36, 255});
    }

    ExportImage(img, path.c_str());
    UnloadImage(img);
    return path;
}

static std::string make_player_image()
{
    const std::string path = "/tmp/zengame_platform_player.png";
    Image img = GenImageColor(32, 48, BLANK);

    ImageDrawRectangle(&img, 8, 8, 16, 24, Color{240, 180, 70, 255});
    ImageDrawRectangle(&img, 10, 32, 5, 12, Color{52, 68, 124, 255});
    ImageDrawRectangle(&img, 17, 32, 5, 12, Color{52, 68, 124, 255});
    ImageDrawRectangle(&img, 6, 12, 20, 8, Color{92, 54, 28, 255});
    ImageDrawRectangle(&img, 9, 4, 14, 8, Color{246, 218, 180, 255});
    ImageDrawCircle(&img, 13, 8, 1, BLACK);
    ImageDrawCircle(&img, 19, 8, 1, BLACK);

    ExportImage(img, path.c_str());
    UnloadImage(img);
    return path;
}

class PaintedStaticBody : public StaticBody2D
{
public:
    Color fill = Color{140, 90, 44, 200};
    Color line = Color{255, 220, 140, 255};

    explicit PaintedStaticBody(const std::string& n) : StaticBody2D(n) {}

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

class PlatformerPlayer : public CharacterBody2D
{
public:
    Vec2 velocity = Vec2();
    float move_input = 0.0f;
    bool jump_requested = false;
    float move_speed = 260.0f;
    float gravity = 1200.0f;
    float jump_speed = 660.0f;
    Sprite2D* visual = nullptr;

    explicit PlatformerPlayer(const std::string& n) : CharacterBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;

        if (c->shape == Collider2D::ShapeType::Circle)
        {
            const Vec2 center = c->get_world_center();
            DrawCircleLines((int)center.x, (int)center.y, c->get_world_radius(), SKYBLUE);
            return;
        }

        if (c->shape == Collider2D::ShapeType::Segment)
        {
            Vec2 a, b;
            c->get_world_segment(a, b);
            DrawLineEx({a.x, a.y}, {b.x, b.y}, 3.0f, SKYBLUE);
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
        velocity.x = move_input * move_speed;
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

        if (visual)
        {
            if (move_input < -0.01f) visual->flip_x = true;
            if (move_input >  0.01f) visual->flip_x = false;
            visual->rotation = 0.0f;
        }
    }
};

static PaintedStaticBody* make_platform(Node* parent, const std::string& name,
                                        float x, float y, float w, float h)
{
    auto* body = new PaintedStaticBody(name);
    body->position = Vec2(x, y);
    body->set_collision_layer_bit(0);
    body->set_collision_mask_bit(0);
    body->fill = Color{134, 92, 52, 220};
    body->line = Color{240, 220, 150, 255};

    auto* col = new Collider2D("Shape");
    col->shape = Collider2D::ShapeType::Rectangle;
    col->size = Vec2(w, h);
    body->add_child(col);

    parent->add_child(body);
    return body;
}

int main()
{
    InitWindow(SCREEN_W, SCREEN_H, "ZenEngine - Platformer move_and_slide Demo");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    const std::string bg_path = make_platformer_bg_image();
    const std::string tile_path = make_platform_tile_image();
    const std::string player_path = make_player_image();

    const int bg_graph = GraphLib::Instance().load("platform_bg", bg_path.c_str());
    const int player_graph = GraphLib::Instance().load("platform_player", player_path.c_str());

    SceneTree tree;
    Node2D* root = new Node2D("Root");

    auto* cam = new View2D("Camera");
    cam->is_current = true;
    root->add_child(cam);

    auto* bg = new Sprite2D("Background");
    bg->graph = bg_graph >= 0 ? bg_graph : 0;
    bg->use_graph_pivot = false;
    bg->pivot = Vec2(0.0f, 0.0f);
    bg->position = Vec2(0.0f, -120.0f);
    root->add_child(bg);

    make_platform(root, "Floor", 0.0f, 620.0f, 2200.0f, 80.0f);
    make_platform(root, "LedgeA", 220.0f, 520.0f, 180.0f, 28.0f);
    make_platform(root, "LedgeB", 520.0f, 440.0f, 200.0f, 28.0f);
    make_platform(root, "LedgeC", 860.0f, 360.0f, 220.0f, 28.0f);
    make_platform(root, "LedgeD", 1240.0f, 470.0f, 240.0f, 28.0f);
    make_platform(root, "LedgeE", 1620.0f, 390.0f, 180.0f, 28.0f);

    auto* ramp = new PaintedStaticBody("Ramp");
    ramp->position = Vec2(1480.0f, 620.0f);
    ramp->set_collision_layer_bit(0);
    ramp->set_collision_mask_bit(0);
    auto* ramp_col = new Collider2D("Shape");
    ramp_col->shape = Collider2D::ShapeType::Polygon;
    ramp_col->points = {
        Vec2(0.0f, 0.0f),
        Vec2(220.0f, -120.0f),
        Vec2(220.0f, 0.0f)
    };
    ramp->add_child(ramp_col);
    root->add_child(ramp);

    auto* rail = new PaintedStaticBody("Rail");
    rail->position = Vec2(1860.0f, 560.0f);
    rail->line = Color{210, 245, 150, 255};
    rail->set_collision_layer_bit(0);
    rail->set_collision_mask_bit(0);
    auto* rail_col = new Collider2D("Shape");
    rail_col->shape = Collider2D::ShapeType::Segment;
    rail_col->points = {Vec2(0.0f, 0.0f), Vec2(180.0f, -90.0f)};
    rail->add_child(rail_col);
    root->add_child(rail);

    auto* player = new PlatformerPlayer("Player");
    player->position = Vec2(240.0f, 340.0f);
    player->set_collision_layer_bit(0);
    player->set_collision_mask_bit(0);
    root->add_child(player);

    auto* player_sprite = new Sprite2D("Visual");
    player_sprite->graph = player_graph >= 0 ? player_graph : 0;
    player_sprite->use_graph_pivot = false;
    player_sprite->pivot = Vec2(0.0f, 0.0f);
    player->add_child(player_sprite);
    player->visual = player_sprite;

    auto* player_col = new Collider2D("Shape");
    player_col->shape = Collider2D::ShapeType::Rectangle;
    player_col->size = Vec2(32.0f, 48.0f);
    player->add_child(player_col);

    tree.set_root(root);
    tree.start();
    tree.set_debug_draw_flags(SceneTree::DEBUG_SHAPES | SceneTree::DEBUG_CONTACTS | SceneTree::DEBUG_NAMES);

    cam->follow_target = player;
    cam->follow_speed = 7.0f;
    cam->position = player->position + Vec2(200.0f, -80.0f);

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        float input = 0.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  input -= 1.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input += 1.0f;
        player->move_input = input;

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        {
            player->jump_requested = true;
        }

        if (IsKeyPressed(KEY_F1))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_SHAPES);
        }
        if (IsKeyPressed(KEY_F2))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_CONTACTS);
        }
        if (IsKeyPressed(KEY_F3))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_NAMES);
        }

        tree.process(dt);

        BeginDrawing();
        ClearBackground(Color{14, 20, 32, 255});
        tree.draw();
        DrawText("A/D or Arrows: move   SPACE/W/UP: jump   F1: shapes   F2: contacts   F3: names", 16, 14, 20, WHITE);
        DrawText(TextFormat("on_floor[%s] vel(%.1f, %.1f)",
                            player->on_floor ? "YES" : "NO",
                            player->velocity.x, player->velocity.y),
                 16, 42, 18, Color{230, 230, 210, 255});
        DrawText("Platformer demo: move_and_slide + procedural textures", 16, 66, 18, Color{220, 220, 255, 255});
        EndDrawing();
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
