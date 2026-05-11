#pragma once

#include "Node2D.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <raylib.h>

// ----------------------------------------------------------------------------
// AnimatedSprite2D
//
// Plays frame-by-frame animations from a named animation library.
// Each animation is a sequence of GraphLib graph IDs played at a given FPS.
//
// Usage:
//   auto* anim = tree.create<AnimatedSprite2D>(root, "Player");
//   anim->add_animation("idle", {idle0, idle1, idle2}, 8.0f);
//   anim->add_animation("run",  {run0, run1, run2, run3}, 12.0f, true);
//   anim->play("idle");
// ----------------------------------------------------------------------------
class AnimatedSprite2D : public Node2D
{
public:
    // One frame in an animation
    struct Frame
    {
        int   graph_id = -1;
        float duration = 0.0f;   // 0 = use animation fps
    };

    struct Animation
    {
        std::vector<Frame> frames;
        float fps  = 12.0f;
        bool  loop = true;
    };

    explicit AnimatedSprite2D(const std::string& p_name = "AnimatedSprite2D");

    // ── Animation library ─────────────────────────────────────────────────────

    // Add an animation from a list of graph IDs at uniform fps.
    void add_animation(const std::string& name,
                       const std::vector<int>& graph_ids,
                       float fps  = 12.0f,
                       bool  loop = true);

    // Add an animation with per-frame durations.
    void add_animation_frames(const std::string& name,
                              const std::vector<Frame>& frames,
                              float default_fps = 12.0f,
                              bool  loop        = true);

    bool has_animation(const std::string& name) const;
    void remove_animation(const std::string& name);

    // ── Playback ──────────────────────────────────────────────────────────────

    void play  (const std::string& name, bool restart = false);
    void stop  ();
    void pause ();
    void resume();

    bool is_playing() const { return m_playing; }
    bool is_paused () const { return m_paused;  }

    const std::string& get_current_animation() const { return m_current; }
    int                get_current_frame()     const { return m_frame;   }

    // Jump to a specific frame (0-based) in the current animation.
    void set_frame(int frame);

    // ── Visual ────────────────────────────────────────────────────────────────
    Color tint       = WHITE;
    bool  flip_h     = false;
    bool  flip_v     = false;
    Vec2  offset     = {0.0f, 0.0f};  // local draw offset

    // ── Callbacks ─────────────────────────────────────────────────────────────
    // Called when a non-looping animation finishes.
    std::function<void(const std::string&)> on_animation_finished;
    // Called every time the frame changes.
    std::function<void(int)> on_frame_changed;

    // ── Node2D overrides ──────────────────────────────────────────────────────
    void _update(float dt) override;
    void _draw  ()         override;

private:
    std::unordered_map<std::string, Animation> m_animations;
    std::string m_current;
    int         m_frame   = 0;
    float       m_elapsed = 0.0f;
    bool        m_playing = false;
    bool        m_paused  = false;

    float frame_duration(const Animation& anim, int frame) const;
};
