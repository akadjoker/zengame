// test_tween.cpp
//
// Visual test for Tween — parallel and sequential modes.
//
// Layout (3 panels):
//
//  LEFT   — Parallel: move + fade + scale all at once
//  MIDDLE — Sequential: slide in → pause → bounce out → call → fade
//  RIGHT  — Sequential loop: infinite ping-pong scale pulse
//
// Controls:
//   SPACE  — restart all tweens
//   1/2/3  — re-trigger left / middle / right tween individually

#include "pch.hpp"
#include <raylib.h>
#include <string>
#include <cmath>

#include "SceneTree.hpp"
#include "Node2D.hpp"
#include "Sprite2D.hpp"
#include "TextNode2D.hpp"
#include "Tween.hpp"

static constexpr int SW = 1280;
static constexpr int SH = 720;

static void label(const char* text, float x, float y, Color c = RAYWHITE)
{
    DrawText(text, (int)x, (int)y, 17, c);
}

static void panel_title(const char* text, float cx)
{
    const int w = MeasureText(text, 22);
    DrawText(text, (int)(cx - w * 0.5f), 14, 22, SKYBLUE);
}

// ─────────────────────────────────────────────────────────────────────────────
// A simple coloured square node
// ─────────────────────────────────────────────────────────────────────────────
class Square : public Node2D
{
public:
    float size  = 60.f;
    Color color = WHITE;

    Square() : Node2D("Square") {}
    explicit Square(const std::string& n) : Node2D(n) {}

    void _draw() override
    {
        Node2D::_draw();
        const Vec2 p = get_global_position();
        const float s = size * scale.x;
        DrawRectangle((int)(p.x - s * 0.5f), (int)(p.y - s * 0.5f),
                      (int)s, (int)s, color);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────
int main()
{
    InitWindow(SW, SH, "Tween test — parallel + sequential");
    SetTargetFPS(60);

    SceneTree tree;
    auto* root = tree.create<Node2D>("root");
    tree.set_root(root);

    const float p1x = SW / 6.f;
    const float p2x = SW * 0.5f;
    const float p3x = SW * 5.f / 6.f;
    const float cy  = SH * 0.5f;

    // ── LEFT — parallel ──────────────────────────────────────────────────────
    auto* sq_par = tree.create<Square>("sq_par");
    root->add_child(sq_par);
    sq_par->position = {p1x, cy};
    sq_par->color    = {255, 120, 60, 255};

    // ── MIDDLE — sequential ──────────────────────────────────────────────────
    auto* sq_seq = tree.create<Square>("sq_seq");
    root->add_child(sq_seq);
    sq_seq->position = {p2x - 200.f, cy};
    sq_seq->color    = {80, 200, 120, 255};

    // status label for middle
    auto* status_label = tree.create<TextNode2D>("status");
    root->add_child(status_label);
    status_label->position = {p2x, cy + 100.f};
    status_label->text     = "waiting...";
    status_label->font_size = 18;
    status_label->color    = LIGHTGRAY;

    // ── RIGHT — sequential loop (ping-pong scale) ────────────────────────────
    auto* sq_loop = tree.create<Square>("sq_loop");
    root->add_child(sq_loop);
    sq_loop->position = {p3x, cy};
    sq_loop->color    = {120, 120, 255, 255};

    // ── Tweens ───────────────────────────────────────────────────────────────

    // LEFT: parallel — all three properties at once
    auto* tw_par = tree.create<Tween>("tw_par");
    root->add_child(tw_par);

    auto setup_parallel = [&]()
    {
        tw_par->kill();
        sq_par->position = {p1x, cy};
        sq_par->scale    = {1.f, 1.f};
        sq_par->color    = {255, 120, 60, 255};
        tw_par->tween_vec2(&sq_par->position, {p1x, cy + 120.f}, {p1x, cy - 120.f}, 1.2f, TweenEase::InOutSine);
        tw_par->tween_float(&sq_par->scale.x, 0.5f, 1.5f, 1.2f, TweenEase::InOutBack);
        tw_par->tween_float(&sq_par->scale.y, 0.5f, 1.5f, 1.2f, TweenEase::InOutBack);
        tw_par->tween_color(&sq_par->color,
                             {255, 120, 60, 255},
                             {255, 220, 50, 255},
                             1.2f, TweenEase::InOutQuad);
        tw_par->loop_mode  = TweenLoop::PingPong;
        tw_par->start();
    };
    setup_parallel();

    // MIDDLE: sequential — slide in → delay → bounce → call → fade out
    auto* tw_seq = tree.create<Tween>("tw_seq");
    root->add_child(tw_seq);
    float seq_alpha = 255.f;  // proxy float for color.a

    auto setup_sequential = [&]()
    {
        tw_seq->kill();
        sq_seq->position = {p2x - 220.f, cy};
        sq_seq->color    = {80, 200, 120, 255};
        sq_seq->scale    = {1.f, 1.f};
        seq_alpha        = 255.f;
        status_label->text = "sliding in...";

        tw_seq->sequential = true;
        tw_seq->tween_vec2(&sq_seq->position,
                            {p2x - 220.f, cy}, {p2x, cy},
                            0.5f, TweenEase::OutCubic)
              .call([&]{ status_label->text = "bounce!"; })
              .tween_float(&sq_seq->scale.x, 1.f, 1.5f, 0.15f, TweenEase::OutBack)
              .tween_float(&sq_seq->scale.x, 1.5f, 1.f, 0.15f, TweenEase::InBack)
              .delay(0.3f)
              .call([&]{ status_label->text = "fading out..."; })
              .tween_lambda([&](float t){
                    seq_alpha = 255.f * (1.f - t);
                    sq_seq->color.a = (unsigned char)(int)seq_alpha;
               }, 0.4f, TweenEase::InQuad)
              .call([&]{ status_label->text = "done! (SPACE to restart)"; });
        tw_seq->loop_mode = TweenLoop::None;
        tw_seq->start();
    };
    setup_sequential();

    // RIGHT: sequential loop — grow → shrink → grow (ping-pong via repeat)
    auto* tw_loop = tree.create<Tween>("tw_loop");
    root->add_child(tw_loop);

    auto setup_loop = [&]()
    {
        tw_loop->kill();
        sq_loop->scale = {1.f, 1.f};
        sq_loop->color = {120, 120, 255, 255};

        tw_loop->sequential = true;
        tw_loop->tween_float(&sq_loop->scale.x, 1.f, 2.f, 0.4f, TweenEase::OutBack)
               .tween_float(&sq_loop->scale.y, 1.f, 2.f, 0.4f, TweenEase::OutBack)
               .tween_color(&sq_loop->color,
                             {120, 120, 255, 255},
                             {255, 80, 180, 255},
                             0.4f)
               .tween_float(&sq_loop->scale.x, 2.f, 1.f, 0.4f, TweenEase::InBack)
               .tween_float(&sq_loop->scale.y, 2.f, 1.f, 0.4f, TweenEase::InBack)
               .tween_color(&sq_loop->color,
                             {255, 80, 180, 255},
                             {120, 120, 255, 255},
                             0.4f);
        tw_loop->loop_mode = TweenLoop::Repeat;
        tw_loop->start();
    };
    setup_loop();

    tree.start();

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        if (IsKeyPressed(KEY_SPACE)) { setup_parallel(); setup_sequential(); setup_loop(); }
        if (IsKeyPressed(KEY_ONE))   setup_parallel();
        if (IsKeyPressed(KEY_TWO))   setup_sequential();
        if (IsKeyPressed(KEY_THREE)) setup_loop();

        tree.process(dt);

        BeginDrawing();
        ClearBackground({18, 18, 28, 255});

        DrawLine(SW / 3, 0, SW / 3, SH, {55, 55, 75, 255});
        DrawLine(2 * SW / 3, 0, 2 * SW / 3, SH, {55, 55, 75, 255});

        panel_title("Parallel",          p1x);
        panel_title("Sequential",        p2x);
        panel_title("Sequential Loop",   p3x);

        // Panel descriptions
        label("move + scale + color", p1x - 80.f, 48.f, GRAY);
        label("slide \u2192 bounce \u2192 fade", p2x - 75.f, 48.f, GRAY);
        label("grow \u2192 shrink \u2192 repeat",  p3x - 80.f, 48.f, GRAY);

        tree.draw();

        label("SPACE=restart all  1/2/3=restart panel", 10, SH - 30, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
