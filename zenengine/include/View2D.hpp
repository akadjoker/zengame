
#pragma once

#include "pch.hpp"
#include "Node2D.hpp"

// ============================================================
// A camera node that defines what region of the world is visible.
// Only ONE camera should be "current" at a time — SceneTree picks
// the first active one it finds in the tree.
//
// Coordinate model (Raylib screen-space, y-down):
//   world_pos  →  screen_pos = (world_pos - camera_pos) * zoom + screen_center
//
// Usage:
//   View2D* cam = new View2D("MainCam");
//   cam->position  = Vec2(0, 0);   // tracks this world position
//   cam->zoom      = 1.5f;
//   cam->is_current = true;
//   root->add_child(cam);
//
// In your game loop, wrap all world draws:
//   auto* cam = tree.get_current_camera();
//   if (cam) cam->begin();
//   tree.draw();
//   if (cam) cam->end();
//
// ============================================================

class View2D : public Node2D
{
public:
    struct CameraShakeState
    {
        bool active = false;
        float amplitudeX = 0.0f;
        float amplitudeY = 0.0f;
        float cycles = 0.0f;
        float omega = 0.0f;
        float cyclesLeft = 0.0f;
    };

    struct TraumaShakeState
    {
        float trauma = 0.0f;
        float decay = 1.5f;
        float amplitudeX = 0.0f;
        float amplitudeY = 0.0f;
        float frequency = 24.0f;
        float time = 0.0f;
    };

    struct ZoomPunchState
    {
        bool active = false;
        float base_zoom = 1.0f;
        float amount = 0.0f;
        float duration = 0.0f;
        float elapsed = 0.0f;
    };

    explicit View2D(const std::string& p_name = "View2D");

    // ----------------------------------------------------------
    // Properties
    // ----------------------------------------------------------

    float zoom       = 1.0f;   // 1.0 = no zoom, 2.0 = 2x closer
    bool  is_current = false;  // only one camera active at a time

    // Screen-space anchor point the camera orbits around.
    // Defaults to centre of the screen set by set_screen_size().
    Vec2  screen_center;       // set automatically by SceneTree

    // Optional smooth-follow target.
    // When set, camera lerps toward target->get_global_position().
    Node2D* follow_target    = nullptr;
    float   follow_speed     = 5.0f;   // lerp factor (units/s)

    // Dead zone: camera only moves when target exits this box
    // (world-space half-extents from camera centre; {0,0} = no dead zone)
    Vec2    dead_zone = Vec2(0.0f, 0.0f);
    Vec2    shake_offset = Vec2(0.0f, 0.0f);
    CameraShakeState shake_state;
    TraumaShakeState trauma_shake;
    ZoomPunchState zoom_punch;

    // ----------------------------------------------------------
    // Runtime API
    // ----------------------------------------------------------

    // Begin/end camera transform (wraps rlPushMatrix / rlPopMatrix)
    void begin() const;
    void end()   const;

    // Convert screen coords → world coords
    Vec2 screen_to_world(const Vec2& screen_pos) const;

    // Convert world coords → screen coords
    Vec2 world_to_screen(const Vec2& world_pos) const;

    // Returns the visible world rectangle (AABB of the viewport)
    void get_viewport_rect(float& out_x, float& out_y,
                           float& out_w, float& out_h) const;
    void start_shake(float amplitude_x, float amplitude_y, float frequency, float duration_cycles);
    void stop_shake();
    void add_trauma(float amount);
    void set_trauma_profile(float amplitude_x, float amplitude_y, float frequency, float decay);
    void clear_trauma();
    void start_zoom_punch(float amount, float duration);
    void stop_zoom_punch();

    // ----------------------------------------------------------
    // Node interface
    // ----------------------------------------------------------

    void _update(float dt) override;

private:

    // Builds the Raylib View2D struct on the fly (zero-overhead)
    void build_rl_camera(::Camera2D& out) const;
    void update_shake(float dt);
    void update_trauma(float dt);
    void update_zoom_punch(float dt);
};
