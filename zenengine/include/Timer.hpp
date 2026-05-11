#pragma once

#include "Node.hpp"
#include <functional>

// ----------------------------------------------------------------------------
// Timer
//
// Counts down and fires on_timeout when it reaches zero.
// Can be one-shot or repeating.
//
// Usage:
//   auto* t = tree.create<Timer>(root, "SpawnTimer");
//   t->wait_time = 2.0f;
//   t->one_shot  = false;   // repeating
//   t->on_timeout = [&]{ spawn_enemy(); };
//   t->start();
// ----------------------------------------------------------------------------
class Timer : public Node
{
public:
    explicit Timer(const std::string& p_name = "Timer");

    // ── Configuration ─────────────────────────────────────────────────────────
    float wait_time = 1.0f;   // seconds until timeout
    bool  one_shot  = false;  // false = repeating
    bool  autostart = false;  // start automatically when added to tree

    // ── Callback ──────────────────────────────────────────────────────────────
    std::function<void()> on_timeout;

    // ── Control ───────────────────────────────────────────────────────────────

    // Start (or restart) the timer. Optionally override wait_time.
    void start(float time = -1.0f);

    // Stop and reset.
    void stop();

    // Pause / resume without resetting.
    void pause ();
    void resume();

    // ── State ─────────────────────────────────────────────────────────────────
    bool  is_stopped () const;
    bool  is_paused  () const { return m_paused; }
    float get_time_left () const { return m_time_left; }
    float get_wait_time () const { return wait_time;   }

    // ── Node override ─────────────────────────────────────────────────────────
    void _ready () override;
    void _update(float dt) override;

private:
    float m_time_left = 0.0f;
    bool  m_running   = false;
    bool  m_paused    = false;
};
