// ─── exemplo de uso ───────────────────────────────────────────────────────────
// main.cpp  (ou o teu game loop)

#include "Light2D.hpp"
#include <raylib.h>

int main()
{
    const int W = 1280, H = 720;
    InitWindow(W, H, "Light2D test");
    SetTargetFPS(60);

    // ── inicializar sistema ─────────────────────────────────────────────────
    LightSystem2D& lights = LightSystem2D::Instance();
    lights.init(W, H);
    lights.debug_lights = true;

    // ── criar luzes ────────────────────────────────────────────────────────
    Light2D sun("sun");
    sun.position  = {400.0f, 300.0f};
    sun.color     = {255, 220, 150, 255};
    sun.radius    = 400.0f;
    sun.intensity = 1.2f;
    lights.register_light(&sun);

    Light2D spot("spot");
    spot.type       = Light2D::Type::Spot;
    spot.position   = {800.0f, 200.0f};
    spot.color      = {100, 180, 255, 255};
    spot.radius     = 350.0f;
    spot.intensity  = 1.5f;
    spot.spot_angle = 30.0f;   // cone de 60° total
    spot.spot_dir   = 90.0f;   // aponta para baixo
    lights.register_light(&spot);

    // ── game loop ───────────────────────────────────────────────────────────
    while (!WindowShouldClose())
    {
        // mover luz com o rato
        sun.position = {(float)GetMouseX(), (float)GetMouseY()};

        // rodar spot
        spot.spot_dir += 30.0f * GetFrameTime();

        BeginDrawing();
        ClearBackground(BLACK);

        // 1. capturar cena
        lights.begin_scene();
        {
            // Desenhas aqui a tua cena normalmente
            // (PolyMesh2D::_draw(), sprites, tiles, etc.)
            DrawRectangle(100, 100, 300, 200, DARKGRAY);
            DrawCircle(600, 400, 60, MAROON);
            DrawText("SCENE", 120, 160, 32, LIGHTGRAY);
        }
        lights.end_scene();

        // 2. composite + luzes → vai para o ecrã
        lights.render({15, 15, 30, 255});   // ambient escuro azulado

        DrawFPS(10, 10);
        EndDrawing();
    }

    lights.unregister_light(&sun);
    lights.unregister_light(&spot);
    lights.shutdown();
    CloseWindow();
    return 0;
}
