#include "pch.hpp"
#include <raylib.h>
#include "SceneTree.hpp"
#include "View2D.hpp"
#include "CollisionObject2D.hpp"
#include "Collider2D.hpp"
#include "Node2D.hpp"
#include "Collision2D.hpp"
#include "Assets.hpp"
#include "CharacterBody2D.hpp"
#include "StaticBody2D.hpp"
#include "Collider2D.hpp"
#include "Sprite2D.hpp"

class DebugCharacter : public CharacterBody2D
{
public:
    explicit DebugCharacter(const std::string& n) : CharacterBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;
        Rectangle r = c->get_world_aabb();
        DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, YELLOW);
    }
};

class DebugStatic : public StaticBody2D
{
public:
    explicit DebugStatic(const std::string& n) : StaticBody2D(n) {}

    void _draw() override
    {
        Collider2D* c = get_collider();
        if (!c) return;
        Rectangle r = c->get_world_aabb();
        DrawRectangle((int)r.x, (int)r.y, (int)r.width, (int)r.height, ColorAlpha(RED, 0.35f));
        DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, RED);
    }
};

int main()
{
    InitWindow(1000, 700, "MiniGodot2D - CharacterBody2D Test");
    SetTargetFPS(60);

    SceneTree tree;

    int coin_id = GraphLib::Instance().load("fire", "../assets/fire.png");

    Node2D* root = new Node2D("Root");

    View2D* cam = new View2D("Camera");
    cam->is_current = true;
    root->add_child(cam);

    DebugCharacter* player = new DebugCharacter("Player");
    player->position = Vec2(120.0f, 120.0f);
    player->set_collision_layer_bit(0);
    player->set_collision_mask_bit(0);
    root->add_child(player);

    Collider2D* player_col = new Collider2D("PlayerCollider");
    player_col->shape = Collider2D::ShapeType::Rectangle;
    player_col->size = Vec2(36.0f, 36.0f);
    player->add_child(player_col);

    DebugStatic* wall1 = new DebugStatic("Wall1");
    wall1->position = Vec2(420.0f, 160.0f);
    wall1->set_collision_layer_bit(0);
    wall1->set_collision_mask_bit(0);
    root->add_child(wall1);

    Collider2D* wall1_col = new Collider2D("Wall1Collider");
    wall1_col->shape = Collider2D::ShapeType::Rectangle;
    wall1_col->size = Vec2(60.0f, 360.0f);
    wall1->add_child(wall1_col);

    DebugStatic* wall2 = new DebugStatic("Wall2");
    wall2->position = Vec2(250.0f, 420.0f);
    wall2->rotation = 25.0f;
    wall2->set_collision_layer_bit(0);
    wall2->set_collision_mask_bit(0);
    root->add_child(wall2);

    Collider2D* wall2_col = new Collider2D("Wall2Collider");
    wall2_col->shape = Collider2D::ShapeType::Polygon;
    wall2_col->points = {Vec2(0.0f, 0.0f), Vec2(220.0f, 20.0f), Vec2(180.0f, 56.0f), Vec2(0.0f, 50.0f)};
    wall2->add_child(wall2_col);


    DebugStatic* coin_body = new DebugStatic("CoinBody");
    coin_body->position = Vec2(200.0f, 100.0f);

    Sprite2D* coin = new Sprite2D("Coin");
    coin->graph = coin_id;
    coin_body->add_child(coin);

    Collider2D* coin_col = new Collider2D("CoinCollider");
    coin_col->shape = Collider2D::ShapeType::Rectangle;
    coin_col->size = Vec2(36.0f, 36.0f);
    coin_body->add_child(coin_col);

    root->add_child(coin_body);

    

    tree.set_root(root);
    tree.start();
    tree.set_debug_draw_flags(SceneTree::DEBUG_PIVOT | SceneTree::DEBUG_DIRECTION |
                              SceneTree::DEBUG_SHAPES | SceneTree::DEBUG_CONTACTS |
                              SceneTree::DEBUG_TREE | SceneTree::DEBUG_NAMES |
                              SceneTree::DEBUG_SPATIAL);

    cam->follow_target = player;
    cam->follow_speed = 6.0f;
    cam->position = player->position;

    while (!WindowShouldClose())
    {
        Vec2 input(0.0f, 0.0f);
        if (IsKeyDown(KEY_RIGHT)) input.x += 1.0f;
        if (IsKeyDown(KEY_LEFT))  input.x -= 1.0f;
        if (IsKeyDown(KEY_DOWN))  input.y += 1.0f;
        if (IsKeyDown(KEY_UP))    input.y -= 1.0f;

        if (input.x != 0.0f || input.y != 0.0f)
        {
            input = input.normalised();
        }

        if (IsKeyPressed(KEY_F1))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_SHAPES);
        }
        if (IsKeyPressed(KEY_F2))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_TREE);
        }
        if (IsKeyPressed(KEY_F3))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_CONTACTS);
        }
        if (IsKeyPressed(KEY_F4))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_NAMES);
        }
        if (IsKeyPressed(KEY_F5))
        {
            tree.set_debug_draw_flags(tree.get_debug_draw_flags() ^ SceneTree::DEBUG_SPATIAL);
        }

        player->move_topdown(input * 220.0f, GetFrameTime());
        tree.process(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);
        tree.draw();

        const uint32_t flags = tree.get_debug_draw_flags();
        const bool dbg_shapes = (flags & SceneTree::DEBUG_SHAPES) != 0;
        const bool dbg_tree = (flags & SceneTree::DEBUG_TREE) != 0;
        const bool dbg_contacts = (flags & SceneTree::DEBUG_CONTACTS) != 0;
        const bool dbg_names = (flags & SceneTree::DEBUG_NAMES) != 0;
        const bool dbg_spatial = (flags & SceneTree::DEBUG_SPATIAL) != 0;

        DrawText("Arrows: move player (move_topdown + place_free)", 12, 12, 20, WHITE);
        DrawText("F1: shapes F2: tree F3: contacts F4: names F5: spatial", 12, 36, 20, WHITE);
        DrawText(TextFormat("Shapes[%s] Tree[%s] Contacts[%s] Names[%s] Spatial[%s]",
                 dbg_shapes ? "ON" : "OFF",
                 dbg_tree ? "ON" : "OFF",
                 dbg_contacts ? "ON" : "OFF",
                 dbg_names ? "ON" : "OFF",
                 dbg_spatial ? "ON" : "OFF"), 12, 60, 18, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
