#include "pch.hpp"
#include <raylib.h>

#include "SceneTree.hpp"
#include "View2D.hpp"
#include "CharacterBody2D.hpp"
#include "Collider2D.hpp"
#include "TileMap2D.hpp"
#include "Assets.hpp"

class DebugCharacter2 : public CharacterBody2D
{
public:
    explicit DebugCharacter2(const std::string& n) : CharacterBody2D(n) {}

    void _draw() override
    {
        if (Collider2D* c = get_collider())
        {
            Rectangle r = c->get_world_aabb();
            DrawRectangle((int)r.x, (int)r.y, (int)r.width, (int)r.height, ColorAlpha(SKYBLUE, 0.45f));
            DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, SKYBLUE);
        }
    }
};

int main()
{
    InitWindow(1100, 720, "MiniGodot2D - TileMap Collision Demo");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    SceneTree tree;
    Node2D* root = new Node2D("Root");

    View2D* cam = new View2D("Camera");
    cam->is_current = true;
    root->add_child(cam);

    TileMap2D* map = new TileMap2D("TileMap");
    map->init(48, 30, 32, 32, 0); // graph 0 = checker default
    map->columns = 1;
    root->add_child(map);

    // Border solids
    for (int x = 0; x < map->width; ++x)
    {
        map->set_tile(x, 0, Tile2D{1, 1});
        map->set_tile(x, map->height - 1, Tile2D{1, 1});
    }
    for (int y = 0; y < map->height; ++y)
    {
        map->set_tile(0, y, Tile2D{1, 1});
        map->set_tile(map->width - 1, y, Tile2D{1, 1});
    }
    // Inner walls
    for (int y = 6; y < 24; ++y)
    {
        map->set_tile(12, y, Tile2D{1, 1});
    }
    for (int x = 18; x < 34; ++x)
    {
        map->set_tile(x, 15, Tile2D{1, 1});
    }

    DebugCharacter2* player = new DebugCharacter2("Player");
    player->position = Vec2(120.0f, 120.0f);
    player->set_collision_layer_bit(0);
    player->set_collision_mask_bit(0);
    root->add_child(player);

    Collider2D* body = new Collider2D("PlayerCollider");
    body->shape = Collider2D::ShapeType::Rectangle;
    body->size = Vec2(24.0f, 24.0f);
    player->add_child(body);

    tree.set_root(root);
    tree.start();
    tree.set_debug_draw_flags(SceneTree::DEBUG_SHAPES | SceneTree::DEBUG_PIVOT | SceneTree::DEBUG_DIRECTION |
                              SceneTree::DEBUG_TREE | SceneTree::DEBUG_SPATIAL);

    cam->follow_target = player;
    cam->follow_speed = 7.0f;
    cam->position = player->position;

    while (!WindowShouldClose())
    {
        Vec2 input(0.0f, 0.0f);
        if (IsKeyDown(KEY_RIGHT)) input.x += 1.0f;
        if (IsKeyDown(KEY_LEFT)) input.x -= 1.0f;
        if (IsKeyDown(KEY_DOWN)) input.y += 1.0f;
        if (IsKeyDown(KEY_UP)) input.y -= 1.0f;
        if (input.x != 0.0f || input.y != 0.0f) input = input.normalised();

        player->move_topdown(input * 220.0f, GetFrameTime());
        tree.process(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);
        tree.draw();
        DrawText("Arrows: move character against solid tiles", 12, 12, 20, WHITE);
        DrawText("Tile collision is checked in CharacterBody2D like old engine collide_with_tiles()", 12, 36, 20, WHITE);
        EndDrawing();
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}

