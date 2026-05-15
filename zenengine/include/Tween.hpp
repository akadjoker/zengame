#pragma once

#include "Node.hpp"
#include "math.hpp"
#include <raylib.h>
#include <vector>
#include <functional>

// ----------------------------------------------------------------------------
// Tween
//
// Interpolates values over time using easing functions.
// Supports float, Vec2 and Color targets, plus generic lambda setters.
//
// PARALLEL mode (default): all tweeners run simultaneously.
// SEQUENTIAL mode (sequential=true): tweeners run one after another.
//   Use delay() to add pauses and call() to fire callbacks mid-sequence.
//
// Usage (parallel):
//   auto* tw = tree.create<Tween>(root, "MoveTween");
//   tw->tween_float(&sprite->position.x, 0.0f, 400.0f, 1.5f, TweenEase::OutQuad);
//   tw->tween_vec2(&sprite->position, {0,0}, {400,300}, 1.0f);
//   tw->on_finished = [&]{ TraceLog(LOG_INFO, "Done!"); };
//
// Usage (sequential):
//   auto* tw = tree.create<Tween>(root, "Intro");
//   tw->sequential = true;
//   tw->tween_vec2(&enemy->position, {-100,300}, {400,300}, 0.4f, TweenEase::OutCubic)
//     .delay(0.1f)
//     .tween_float(&enemy->color.a, 255, 0, 0.3f)
//     .call([&]{ enemy->visible = false; });
//   tw->start();
// ----------------------------------------------------------------------------

enum class TweenEase : uint8_t
{
    Linear = 0,
    InQuad,   OutQuad,   InOutQuad,
    InCubic,  OutCubic,  InOutCubic,
    InQuart,  OutQuart,  InOutQuart,
    InSine,   OutSine,   InOutSine,
    InBack,   OutBack,   InOutBack,
    InBounce, OutBounce, InOutBounce,
    InElastic,OutElastic,InOutElastic
};

enum class TweenLoop : uint8_t
{
    None    = 0,  // play once
    Repeat,       // restart from beginning
    PingPong      // reverse direction each cycle
};

class Tween : public Node
{
public:
    explicit Tween(const std::string& p_name = "Tween");

    // ── Tweeners ──────────────────────────────────────────────────────────────

    // Tween a float from `from` to `to` over `duration` seconds.
    Tween& tween_float(float* target, float from, float to,
                       float duration, TweenEase ease = TweenEase::Linear);

    // Tween a Vec2 component-wise.
    Tween& tween_vec2(Vec2* target, Vec2 from, Vec2 to,
                      float duration, TweenEase ease = TweenEase::Linear);

    // Tween a Color (RGBA lerp).
    Tween& tween_color(Color* target, Color from, Color to,
                       float duration, TweenEase ease = TweenEase::Linear);

    // Generic: receives the eased [0..1] parameter each frame.
    Tween& tween_lambda(std::function<void(float)> setter,
                        float duration, TweenEase ease = TweenEase::Linear);

    // Sequential only: pause for `duration` seconds before the next step.
    Tween& delay(float duration);

    // Sequential only: fire a callback instantly at this point in the sequence.
    Tween& call(std::function<void()> fn);

    // ── Control ───────────────────────────────────────────────────────────────
    void start   ();    // (re)start from beginning
    void stop    ();
    void pause   ();
    void resume  ();
    void kill    ();    // stop and clear all tweeners

    bool is_running() const { return m_running && !m_paused; }
    bool is_paused () const { return m_paused;  }
    bool is_valid  () const { return !m_tweeners.empty(); }

    // ── Settings ──────────────────────────────────────────────────────────────
    TweenLoop loop_mode  = TweenLoop::None;
    bool      sequential = false;  // if true, steps run one after another

    // ── Callbacks ─────────────────────────────────────────────────────────────
    std::function<void()> on_finished;   // called when all tweeners complete
    std::function<void()> on_step;       // called every update tick

    // ── Node override ─────────────────────────────────────────────────────────
    void _update(float dt) override;

private:
    // Internal tweener descriptor
    struct Tweener
    {
        std::function<void(float)> setter;  // receives eased [0..1]
        float duration = 1.0f;
        TweenEase ease = TweenEase::Linear;
    };

    std::vector<Tweener> m_tweeners;
    float m_elapsed  = 0.0f;
    float m_duration = 0.0f;   // max duration (parallel) or unused (sequential)
    int   m_step     = 0;      // current step index (sequential mode only)
    bool  m_running  = false;
    bool  m_paused   = false;
    bool  m_reverse  = false;   // for PingPong

    static float apply_ease(float t, TweenEase ease);
    void recalc_duration();
};
