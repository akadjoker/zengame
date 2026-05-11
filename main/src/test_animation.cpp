#include "SceneTree.hpp"
#include "AnimatedSprite2D.hpp"
#include "TextNode2D.hpp"
#include "Timer.hpp"
#include "Tween.hpp"
#include "View2D.hpp"
#include "Assets.hpp"
#include <raylib.h>
#include <string>

// ----------------------------------------------------------------------------
// Sprite sheet layout (adjust to your actual assets)
// ----------------------------------------------------------------------------
static constexpr int   RUN_FRAMES  = 6;
static constexpr int   IDLE_FRAMES = 5;
static constexpr int   DIE_FRAMES  = 4;

static const char* ASSET_RUN  = "../assets/Gunner_Red_Run.png";
static const char* ASSET_IDLE = "../assets/Gunner_Red_Idle.png";
static const char* ASSET_DIE  = "../assets/Gunner_Red_Death.png";

// ============================================================================
// Demo scene
// ============================================================================
class AnimationDemo : public Node
{
public:
    AnimatedSprite2D* character = nullptr;
    TextNode2D*       info_label = nullptr;
    Tween*            move_tween = nullptr;
    Timer*            anim_timer = nullptr;

    int               anim_idx   = 0;
    std::vector<std::string> anim_names = {"idle", "run", "die"};

    explicit AnimationDemo(const std::string& name = "AnimationDemo") : Node(name) {}

    void _ready() override
    {
        // ── Camera ────────────────────────────────────────────────────────────
        auto* cam = m_tree->create<View2D>(this, "Camera");
        cam->is_current = true;
        cam->zoom = 2.0f;
        cam->position = Vec2(400, 300);

        // ── Load sprite sheets as atlases ─────────────────────────────────────
        GraphLib& lib = GraphLib::Instance();

        std::vector<int> run_ids, idle_ids, die_ids;

        // Horizontal strip: loadAtlas(name, path, cols, rows)
        const int run_first  = lib.loadAtlas("run",  ASSET_RUN,  RUN_FRAMES,  1);
        const int idle_first = lib.loadAtlas("idle", ASSET_IDLE, IDLE_FRAMES, 1);
        const int die_first  = lib.loadAtlas("die",  ASSET_DIE,  DIE_FRAMES,  1);

        for (int i = 0; i < RUN_FRAMES;  ++i) run_ids.push_back(run_first  + i);
        for (int i = 0; i < IDLE_FRAMES; ++i) idle_ids.push_back(idle_first + i);
        for (int i = 0; i < DIE_FRAMES;  ++i) die_ids.push_back(die_first  + i);

        // ── AnimatedSprite2D ──────────────────────────────────────────────────
        character = m_tree->create<AnimatedSprite2D>(this, "Character");
        character->position = Vec2(200, 300);
        character->add_animation("idle", idle_ids, 10.0f, true);
        character->add_animation("run",  run_ids,  12.0f, true);
        character->add_animation("die",  die_ids,   8.0f, false);

        character->on_animation_finished = [&](const std::string& name)
        {
            if (name == "die")
            {
                character->play("idle");
                anim_idx = 0;
            }
        };

        character->play("idle");

        // ── Timer: cycle animations every 2 seconds ───────────────────────────
        anim_timer = m_tree->create<Timer>(this, "AnimTimer");
        anim_timer->wait_time = 2.0f;
        anim_timer->one_shot  = false;
        anim_timer->on_timeout = [&]
        {
            anim_idx = (anim_idx + 1) % (int)anim_names.size();
            character->play(anim_names[anim_idx], true);
        };
        anim_timer->start();

        // ── Tween: move character back and forth ──────────────────────────────
        move_tween = m_tree->create<Tween>(this, "MoveTween");
        move_tween->tween_vec2(
            &character->position,
            Vec2(100, 300), Vec2(600, 300),
            3.0f, TweenEase::InOutSine);
        move_tween->loop_mode = TweenLoop::PingPong;
        move_tween->on_step = [&]
        {
            // Flip sprite based on movement direction
            // (PingPong reversal detected via tween's elapsed via lambda capture is tricky,
            //  so we just track position delta)
        };
        move_tween->start();

        // ── TextNode2D: info HUD (via ui_root set below) ──────────────────────
        info_label = m_tree->create<TextNode2D>(this, "InfoLabel");
        info_label->position  = Vec2(10, 10);
        info_label->font_size = 18.0f;
        info_label->color     = WHITE;
        info_label->text      = "Loading...";
    }

    void _update(float dt) override
    {
        (void)dt;

        // Manual animation switch with keys
        if (IsKeyPressed(KEY_ONE))   { character->play("idle", true); anim_idx = 0; }
        if (IsKeyPressed(KEY_TWO))   { character->play("run",  true); anim_idx = 1; }
        if (IsKeyPressed(KEY_THREE)) { character->play("die",  true); anim_idx = 2; }

        // Pause / resume tween
        if (IsKeyPressed(KEY_SPACE))
        {
            if (move_tween->is_paused()) move_tween->resume();
            else                         move_tween->pause();
        }

        // Pause / resume timer
        if (IsKeyPressed(KEY_T))
        {
            if (anim_timer->is_paused()) anim_timer->resume();
            else                          anim_timer->pause();
        }

        // Flip
        if (IsKeyPressed(KEY_F))
            character->flip_h = !character->flip_h;

        // Update info
        const std::string paused_t = anim_timer->is_paused() ? " [PAUSED]" : "";
        const std::string paused_m = move_tween->is_paused() ? " [PAUSED]" : "";
        info_label->text =
            "Anim: "  + character->get_current_animation() +
            "  Frame: " + std::to_string(character->get_current_frame()) +
            "\nTimer: " + std::to_string((int)(anim_timer->get_time_left() * 10) / 10.0f) + "s" + paused_t +
            "\nTween: " + (move_tween->is_running() ? "running" : "paused") + paused_m +
            "\n\n1/2/3: anim  SPACE: pause tween  T: pause timer  F: flip";
    }
};

// ============================================================================
// Main
// ============================================================================
int main()
{
    InitWindow(800, 600, "ZenEngine - AnimatedSprite2D + Timer + Tween");
    SetTargetFPS(60);

    GraphLib::Instance().create();

    SceneTree tree;
    tree.set_root(tree.create<AnimationDemo>());
    tree.start();

    while (tree.is_running() && !WindowShouldClose())
    {
        tree.process(GetFrameTime());
        BeginDrawing();
        ClearBackground(Color{30, 30, 40, 255});
        tree.draw();
        EndDrawing();
    }

    tree.quit();
    GraphLib::Instance().destroy();
    CloseWindow();
    return 0;
}
