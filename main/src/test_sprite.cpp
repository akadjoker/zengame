#include "pch.hpp"
#include "Node.hpp"
#include "Node2D.hpp"
#include "SceneTree.hpp"
#include "Sprite2D.hpp"
#include "View2D.hpp"
#include "math.hpp"
#include "Assets.hpp"
#include "SpatialHash2D.hpp"

int main()
{
    InitWindow(800, 600, "MiniGodot2D - Sprite Test");
    SetTargetFPS(60);

    SceneTree tree;
    SpatialHash2D hash(96.0f);

    GraphLib::Instance().create();

    int player_id = GraphLib::Instance().load("player", "../assets/playerShip1_orange.png");
    int coin_id = GraphLib::Instance().load("fire", "../assets/fire.png");

    // Cria cena
    Node2D *root = new Node2D("Root");
    View2D *view = new View2D("View");
    view->zoom = 1.0f;
    view->is_current = true;
    root->add_child(view);

    Sprite2D *player = new Sprite2D("Player");
    player->graph = player_id;
    player->position = Vec2(400, 300);
    root->add_child(player);

    Sprite2D *coin = new Sprite2D("Coin");
    coin->graph = coin_id; // frame 0 do atlas
    coin->position = Vec2(200, 200);
    root->add_child(coin);

    tree.set_root(root); 
    tree.start();

    view->position = player->position;
    view->follow_target = player;
    view->follow_speed = 5.0f;

    while (!WindowShouldClose())
    {
        
        if (IsKeyDown(KEY_RIGHT)) view->position.x += 200.0f * GetFrameTime();
        if (IsKeyDown(KEY_LEFT))  view->position.x -= 200.0f * GetFrameTime();
        if (IsKeyDown(KEY_DOWN))  view->position.y += 200.0f * GetFrameTime();
        if (IsKeyDown(KEY_UP))    view->position.y -= 200.0f * GetFrameTime();

        if (IsKeyDown(KEY_P)) view->zoom += 1.0f * GetFrameTime();
        if (IsKeyDown(KEY_L)) view->zoom -= 1.0f * GetFrameTime();
        if (IsKeyDown(KEY_O)) view->rotation -= 90.0f * GetFrameTime();
        if (IsKeyDown(KEY_K)) view->rotation += 90.0f * GetFrameTime();

        if (IsKeyDown(KEY_A)) player->rotation -= 90.0f * GetFrameTime();
        if (IsKeyDown(KEY_D)) player->rotation += 90.0f * GetFrameTime();
        if (IsKeyDown(KEY_W)) player->advance(125.0f * GetFrameTime());
        if (IsKeyDown(KEY_S)) player->advance(125.0f * GetFrameTime(), 180.0f);

        float dt = GetFrameTime();
        tree.process(dt);

        float px, py, pw, ph;
        float cx, cy, cw, ch;
        player->get_bounds(px, py, pw, ph);
        coin->get_bounds(cx, cy, cw, ch);

        hash.upsert(SpatialBody2D{1, player, Rectangle{px, py, pw, ph}, 1u << 0, 0xFFFFFFFFu});
        hash.upsert(SpatialBody2D{2, coin,   Rectangle{cx, cy, cw, ch}, 1u << 0, 0xFFFFFFFFu});

        std::vector<std::pair<int, int>> pairs;
        hash.query_pairs(pairs);

        BeginDrawing();
        ClearBackground(BLACK);
        tree.draw();

        static bool show_debug = true;
        if (IsKeyPressed(KEY_F1)) show_debug = !show_debug;
        if (show_debug)
        {
            View2D* cam = tree.get_current_camera();
            if (cam)
            {
                cam->begin();
                hash.debug_draw_cells(Color{80, 180, 70, 140});
                hash.debug_draw_bodies(pairs.empty() ? YELLOW : RED);
                cam->end();
            }

            DrawRectangle(8, 8, 370, 80, ColorAlpha(BLACK, 0.65f));
            DrawText("F1 Debug SpatialHash", 16, 16, 20, WHITE);
            DrawText(TextFormat("Pairs: %d", (int)pairs.size()), 16, 42, 20, pairs.empty() ? GREEN : RED);
        }
        EndDrawing();

        
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
