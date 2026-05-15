// test_drawnode.cpp
//
// Visual test for DrawNode2D — all primitives in one screen.
//
// Controls:
//   SPACE — toggle animated arrow that follows mouse
//   R     — reset / clear and redraw static shapes

#include "pch.hpp"
#include <raylib.h>
#include <cmath>
#include <string>

#include "SceneTree.hpp"
#include "Node2D.hpp"
#include "DrawNode2D.hpp"

static constexpr int SW = 1280;
static constexpr int SH = 720;

static void label(const char* t, float x, float y, Color c = LIGHTGRAY)
{
    DrawText(t, (int)x, (int)y, 16, c);
}

int main()
{
    InitWindow(SW, SH, "DrawNode2D test");
    SetTargetFPS(60);

    SceneTree tree;
    auto* root = tree.create<Node2D>("root");
    tree.set_root(root);

    auto* d = tree.create<DrawNode2D>("draw");
    root->add_child(d);

    bool show_mouse_arrow = true;
    float t = 0.f;

    tree.start();

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        t += dt;

        if (IsKeyPressed(KEY_SPACE)) show_mouse_arrow = !show_mouse_arrow;

        // Rebuild draw list every frame
        d->clear();

        // ── Grid lines ───────────────────────────────────────────────────────
        for (int x = 0; x <= SW; x += 160)
            d->draw_line({(float)x, 0}, {(float)x, (float)SH}, 1.f, {40,40,60,255});
        for (int y = 0; y <= SH; y += 120)
            d->draw_line({0, (float)y}, {(float)SW, (float)y}, 1.f, {40,40,60,255});

        // ── Lines ────────────────────────────────────────────────────────────
        d->draw_line({20, 60}, {200, 60}, 2.f, WHITE);
        d->draw_line({20, 80}, {200, 80}, 4.f, YELLOW);
        d->draw_line({20, 100}, {200, 100}, 8.f, RED);

        // ── Polyline ─────────────────────────────────────────────────────────
        Vec2 poly[] = {{230,40},{280,100},{340,50},{390,110},{440,40}};
        d->draw_polyline(poly, 5, 2.f, SKYBLUE);

        // ── Closed polyline ───────────────────────────────────────────────────
        Vec2 cpoly[] = {{490,40},{560,40},{590,100},{520,130},{460,100}};
        d->draw_polyline(cpoly, 5, 2.f, LIME, true);

        // ── Circles ──────────────────────────────────────────────────────────
        d->draw_circle      ({100, 220}, 50, {255, 80, 80, 180});
        d->draw_circle_lines({100, 220}, 50, 2.f, RED);

        d->draw_circle      ({220, 220}, 40, {80, 200, 255, 200});
        d->draw_circle_lines({220, 220}, 40, 3.f, SKYBLUE);

        // ── Arc ──────────────────────────────────────────────────────────────
        d->draw_arc({340, 220}, 50, 0, 270, 32, 3.f, ORANGE);
        d->draw_arc({450, 220}, 50, -90 + t * 60.f, 180 + t * 60.f, 24, 2.f, PINK);

        // ── Rects ────────────────────────────────────────────────────────────
        d->draw_rect      ({560, 170}, {700, 270}, {120, 255, 120, 160});
        d->draw_rect_lines({560, 170}, {700, 270}, 2.f, GREEN);

        d->draw_rect_lines({720, 170}, {860, 270}, 4.f, GOLD);

        // ── Triangles ────────────────────────────────────────────────────────
        d->draw_triangle      ({900, 170}, {980, 270}, {830, 270}, {255,180,60,200});
        d->draw_triangle_lines({1010,170},{1090,270},{950,270}, 2.f, YELLOW);

        // ── Dots ─────────────────────────────────────────────────────────────
        for (int i = 0; i < 8; ++i)
        {
            const float ang = i * (M_PI_F * 2.f / 8.f) + t;
            Vec2 p = {120.f + cosf(ang) * 60.f, 400.f + sinf(ang) * 60.f};
            d->draw_dot(p, 6.f + sinf(t * 3 + i) * 3.f,
                        {(uint8_t)(200+i*7), (uint8_t)(80+i*20), 200, 255});
        }

        // ── Filled polygon ───────────────────────────────────────────────────
        {
            const int  sides = 6;
            Vec2 hex[6];
            for (int i = 0; i < sides; ++i)
            {
                const float a = i * (M_PI_F * 2.f / sides) - M_PI_F / 2.f;
                hex[i] = {300.f + cosf(a) * 55.f, 400.f + sinf(a) * 55.f};
            }
            d->draw_polygon      (hex, sides, {100, 180, 255, 160});
            d->draw_polygon_lines(hex, sides, 2.f, WHITE);
        }

        // ── Rotating star (polygon) ───────────────────────────────────────────
        {
            const int pts_n = 10;
            Vec2 star[10];
            for (int i = 0; i < pts_n; ++i)
            {
                const float a = i * (M_PI_F * 2.f / pts_n) + t * 0.8f;
                const float r = (i % 2 == 0) ? 60.f : 25.f;
                star[i] = {500.f + cosf(a) * r, 400.f + sinf(a) * r};
            }
            d->draw_polygon      (star, pts_n, {255, 220, 50, 180});
            d->draw_polygon_lines(star, pts_n, 1.5f, GOLD);
        }

        // ── Arrows ───────────────────────────────────────────────────────────
        d->draw_arrow({680, 340}, {780, 390}, 2.f, 14.f, WHITE);
        d->draw_arrow({780, 390}, {830, 320}, 3.f, 18.f, ORANGE);

        // ── Animated arrow toward mouse ───────────────────────────────────────
        if (show_mouse_arrow)
        {
            Vector2 mp = GetMousePosition();
            d->draw_arrow({(float)SW*0.5f, (float)SH*0.5f},
                          {mp.x, mp.y}, 3.f, 20.f,
                          {255, 80, 80, 200});
        }

        // ── Pulsing ring ─────────────────────────────────────────────────────
        {
            const float pulse = 50.f + sinf(t * 3.f) * 20.f;
            const unsigned char a = (unsigned char)(180 + sinf(t * 3.f) * 70);
            d->draw_circle_lines({1100, 400}, pulse, 3.f, {80, 255, 180, a});
        }

        // ── HUD labels ───────────────────────────────────────────────────────
        tree.process(dt);

        BeginDrawing();
        ClearBackground({12, 12, 22, 255});
        tree.draw();

        label("lines / polylines",      20,  20);
        label("circles + arcs",         20, 145);
        label("rects",                  20, 295);
        label("triangles",              20, 345);
        label("dots orbit",             20, 455);
        label("hexagon / star",        260, 455);
        label("arrows",                660, 310);
        label("pulsing ring",         1060, 460);
        label("SPACE=toggle mouse arrow", 10, SH-28, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
