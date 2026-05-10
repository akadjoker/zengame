#include "pch.hpp"
#include <raylib.h>

#include "Assets.hpp"
#include "Node2D.hpp"
#include "Parallax2D.hpp"
#include "ParallaxLayer2D.hpp"
#include "SceneTree.hpp"
#include "Sprite2D.hpp"
#include "View2D.hpp"

static std::string make_sky_parallax_image()
{
    const std::string path = "/tmp/zengame_parallax_sky.png";
    Image img = GenImageColor(512, 256, Color{18, 28, 52, 255});

    for (int y = 0; y < img.height; ++y)
    {
        const float t = (float)y / (float)(img.height - 1);
        const Color c = {
            (unsigned char)(18 + t * 20.0f),
            (unsigned char)(28 + t * 36.0f),
            (unsigned char)(52 + t * 60.0f),
            255
        };
        ImageDrawLine(&img, 0, y, img.width - 1, y, c);
    }

    for (int i = 0; i < 80; ++i)
    {
        const int x = (i * 47) % img.width;
        const int y = (i * 29) % (img.height / 2);
        const int r = 1 + (i % 2);
        ImageDrawCircle(&img, x, y, r, Color{255, 245, 210, 220});
    }

    ExportImage(img, path.c_str());
    UnloadImage(img);
    return path;
}

static std::string make_mountain_parallax_image()
{
    const std::string path = "/tmp/zengame_parallax_mountains.png";
    Image img = GenImageColor(512, 196, BLANK);

    const int ridge_y = 132;
    for (int x = -20; x < img.width + 40; x += 58)
    {
        const int h = 26 + ((x + 20) / 58 % 3) * 20;
        ImageDrawCircle(&img, x + 28, ridge_y - h, 34, Color{70, 90, 130, 255});
        ImageDrawRectangle(&img, x, ridge_y - h, 56, h + 64, Color{70, 90, 130, 255});
    }

    ImageDrawRectangle(&img, 0, ridge_y + 32, img.width, img.height - (ridge_y + 32), Color{28, 54, 104, 255});

    ExportImage(img, path.c_str());
    UnloadImage(img);
    return path;
}

static std::string make_near_parallax_image()
{
    const std::string path = "/tmp/zengame_parallax_near.png";
    Image img = GenImageColor(512, 256, BLANK);

    ImageDrawRectangle(&img, 0, 180, img.width, 76, Color{48, 70, 42, 255});
    for (int x = -30; x < img.width + 40; x += 56)
    {
        const int top = 126 + ((x + 30) / 56 % 4) * 10;
        ImageDrawCircle(&img, x + 28, top, 30, Color{72, 108, 58, 255});
        ImageDrawRectangle(&img, x, top, 56, 120, Color{72, 108, 58, 255});
    }

    for (int x = 20; x < img.width; x += 96)
    {
        ImageDrawRectangle(&img, x, 110, 10, 110, Color{55, 35, 20, 255});
        ImageDrawCircle(&img, x + 5, 92, 26, Color{88, 135, 72, 255});
        ImageDrawCircle(&img, x - 10, 104, 18, Color{88, 135, 72, 255});
        ImageDrawCircle(&img, x + 20, 104, 18, Color{88, 135, 72, 255});
    }

    ExportImage(img, path.c_str());
    UnloadImage(img);
    return path;
}

int main()
{
    InitWindow(1200, 760, "MiniGodot2D - Parallax2D Test");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    const std::string sky_path = make_sky_parallax_image();
    const std::string mountain_path = make_mountain_parallax_image();
    const std::string near_path = make_near_parallax_image();

    const int bg_sky = GraphLib::Instance().load("bg_sky", sky_path.c_str());
    const int bg_mountains = GraphLib::Instance().load("bg_mountains", mountain_path.c_str());
    const int bg_near = GraphLib::Instance().load("bg_near", near_path.c_str());
    const int ship = GraphLib::Instance().load("ship", "../assets/playerShip1_orange.png");
    const int fire = GraphLib::Instance().load("fire", "../assets/fire.png");

    SceneTree tree;
    Node2D* root = new Node2D("Root");

    View2D* cam = new View2D("Camera");
    cam->is_current = true;
    root->add_child(cam);

    Parallax2D* parallax = new Parallax2D("Parallax");
    root->add_child(parallax);

    ParallaxLayer2D* sky = new ParallaxLayer2D("SkyBackground");
    sky->graph = bg_sky >= 0 ? bg_sky : 0;
    sky->scroll_factor_x = 0.10f;
    sky->scroll_factor_y = 0.0f;
    sky->use_camera_y = false;
    sky->mode = ParallaxLayer2D::TILE_X | ParallaxLayer2D::STRETCH_Y;
    sky->size.y = 760.0f;
    sky->color = Color{180, 200, 220, 255};
    parallax->add_child(sky);

    ParallaxLayer2D* far = new ParallaxLayer2D("FarMountains");
    far->graph = bg_mountains >= 0 ? bg_mountains : 0;
    far->scroll_factor_x = 0.15f;
    far->scroll_factor_y = 0.05f;
    far->mode = ParallaxLayer2D::TILE_X;
    far->size.y = 196.0f;
    far->color = Color{180, 200, 220, 255};
    parallax->add_child(far);

    ParallaxLayer2D* near = new ParallaxLayer2D("NearBackground");
    near->graph = bg_near >= 0 ? bg_near : 0;
    near->scroll_factor_x = 0.45f;
    near->scroll_factor_y = 0.15f;
    near->mode = ParallaxLayer2D::TILE_X;
    near->size.y = 256.0f;
    near->color = Color{255, 255, 255, 190};
    parallax->add_child(near);

    ParallaxLayer2D* far_objects = new ParallaxLayer2D("FarObjects");
    far_objects->scroll_factor_x = 0.35f;
    far_objects->scroll_factor_y = 0.20f;
    parallax->add_child(far_objects);

    Sprite2D* far_glow_a = new Sprite2D("FarGlowA");
    far_glow_a->graph = fire >= 0 ? fire : 0;
    far_glow_a->position = Vec2(180.0f, 180.0f);
    far_glow_a->scale = Vec2(2.2f, 2.2f);
    far_glow_a->color = Color{255, 180, 120, 180};
    far_objects->add_child(far_glow_a);

    Sprite2D* far_glow_b = new Sprite2D("FarGlowB");
    far_glow_b->graph = fire >= 0 ? fire : 0;
    far_glow_b->position = Vec2(920.0f, 260.0f);
    far_glow_b->scale = Vec2(1.8f, 1.8f);
    far_glow_b->color = Color{255, 210, 140, 170};
    far_objects->add_child(far_glow_b);

    ParallaxLayer2D* mid_objects = new ParallaxLayer2D("MidObjects");
    mid_objects->scroll_factor_x = 0.65f;
    mid_objects->scroll_factor_y = 0.45f;
    parallax->add_child(mid_objects);

    Sprite2D* mid_ship_a = new Sprite2D("MidShipA");
    mid_ship_a->graph = ship >= 0 ? ship : 0;
    mid_ship_a->position = Vec2(260.0f, 420.0f);
    mid_ship_a->scale = Vec2(1.6f, 1.6f);
    mid_ship_a->rotation = -8.0f;
    mid_objects->add_child(mid_ship_a);

    Sprite2D* mid_ship_b = new Sprite2D("MidShipB");
    mid_ship_b->graph = ship >= 0 ? ship : 0;
    mid_ship_b->position = Vec2(980.0f, 500.0f);
    mid_ship_b->scale = Vec2(1.2f, 1.2f);
    mid_ship_b->rotation = 12.0f;
    mid_objects->add_child(mid_ship_b);

    Sprite2D* player = new Sprite2D("Player");
    player->graph = ship >= 0 ? ship : 0;
    player->position = Vec2(400.0f, 300.0f);
    root->add_child(player);

    tree.set_root(root);
    tree.start();

    cam->follow_target = player;
    cam->follow_speed = 8.0f;
    cam->position = player->position;

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player->position.x -= 260.0f * dt;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player->position.x += 260.0f * dt;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) player->position.y -= 260.0f * dt;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) player->position.y += 260.0f * dt;

        tree.process(dt);

        BeginDrawing();
        ClearBackground(BLACK);
        tree.draw();
        DrawText("WASD/Arrows move player. Procedural backgrounds + parallax layers.", 12, 12, 20, WHITE);
        EndDrawing();
    }

    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
