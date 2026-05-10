
#include "pch.hpp"
#include <raylib.h>
#include "View2D.hpp"

// ============================================================
// View2D.cpp
// ============================================================

View2D::View2D(const std::string& p_name)
    : Node2D(p_name)
    , screen_center(0.0f, 0.0f)
{
    zoom_punch.base_zoom = zoom;
}

// ----------------------------------------------------------
// Internal: build Raylib View2D from our state
// ----------------------------------------------------------

void View2D::build_rl_camera(::Camera2D& out) const
{
    Vec2 world_pos = get_global_position();

    // Raylib Camera2D fields:
    //   offset  — screen-space anchor (usually screen centre)
    //   target  — world-space position the anchor maps to
    //   rotation, zoom
    out.offset   = { screen_center.x + shake_offset.x, screen_center.y + shake_offset.y };
    out.target   = { world_pos.x,     world_pos.y     };
    out.rotation = rotation;   
    out.zoom     = zoom;
}

// ----------------------------------------------------------
// begin / end  —  wrap all world-space draws between these
// ----------------------------------------------------------

void View2D::begin() const
{
    ::Camera2D rl_cam;
    build_rl_camera(rl_cam);
    BeginMode2D(rl_cam);
}

void View2D::end() const
{
    EndMode2D();
}

// ----------------------------------------------------------
// Coordinate helpers
// ----------------------------------------------------------

Vec2 View2D::screen_to_world(const Vec2& screen_pos) const
{
    ::Camera2D rl_cam;
    build_rl_camera(rl_cam);
    Vector2 w = GetScreenToWorld2D({ screen_pos.x, screen_pos.y }, rl_cam);
    return Vec2(w.x, w.y);
}

Vec2 View2D::world_to_screen(const Vec2& world_pos) const
{
    ::Camera2D rl_cam;
    build_rl_camera(rl_cam);
    Vector2 s = GetWorldToScreen2D({ world_pos.x, world_pos.y }, rl_cam);
    return Vec2(s.x, s.y);
}

void View2D::get_viewport_rect(float& out_x, float& out_y,
                                  float& out_w, float& out_h) const
{
    Vec2 tl = screen_to_world(Vec2(0.0f, 0.0f));
    Vec2 br = screen_to_world(Vec2(screen_center.x * 2.0f,
                                   screen_center.y * 2.0f));
    out_x = tl.x;
    out_y = tl.y;
    out_w = br.x - tl.x;
    out_h = br.y - tl.y;
}

void View2D::start_shake(float amplitude_x, float amplitude_y, float frequency, float duration_cycles)
{
    if (frequency <= 0.0f || duration_cycles <= 0.0f)
    {
        stop_shake();
        return;
    }

    shake_state.active = true;
    shake_state.amplitudeX = amplitude_x;
    shake_state.amplitudeY = amplitude_y;
    shake_state.cycles = duration_cycles;
    shake_state.omega = frequency * 2.0f * M_PI_F;
    shake_state.cyclesLeft = duration_cycles;
}

void View2D::stop_shake()
{
    shake_state.active = false;
    shake_state.cyclesLeft = 0.0f;
}

void View2D::update_shake(float dt)
{
    if (!shake_state.active)
    {
        return;
    }

    const float frequency = shake_state.omega / (2.0f * M_PI_F);
    shake_state.cyclesLeft -= dt * frequency;

    if (shake_state.cyclesLeft > 0.0f && shake_state.cycles > 0.0f)
    {
        const float frac = shake_state.cyclesLeft / shake_state.cycles;
        const float elapsed_cycles = shake_state.cycles - shake_state.cyclesLeft;
        const float v = frac * frac * std::cos(elapsed_cycles * 2.0f * M_PI_F);

        const float sign_x = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;
        const float sign_y = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;

        shake_offset.x = shake_state.amplitudeX * sign_x * v;
        shake_offset.y = shake_state.amplitudeY * sign_y * v;
    }
    else
    {
        shake_state.active = false;
    }
}

void View2D::add_trauma(float amount)
{
    if (amount <= 0.0f)
    {
        return;
    }

    trauma_shake.trauma += amount;
    if (trauma_shake.trauma > 1.0f)
    {
        trauma_shake.trauma = 1.0f;
    }
}

void View2D::set_trauma_profile(float amplitude_x, float amplitude_y, float frequency, float decay)
{
    trauma_shake.amplitudeX = amplitude_x;
    trauma_shake.amplitudeY = amplitude_y;
    trauma_shake.frequency = frequency;
    trauma_shake.decay = decay;
}

void View2D::clear_trauma()
{
    trauma_shake.trauma = 0.0f;
    trauma_shake.time = 0.0f;
}

void View2D::start_zoom_punch(float amount, float duration)
{
    if (duration <= 0.0f)
    {
        return;
    }

    zoom_punch.active = true;
    zoom_punch.base_zoom = zoom;
    zoom_punch.amount = amount;
    zoom_punch.duration = duration;
    zoom_punch.elapsed = 0.0f;
}

void View2D::stop_zoom_punch()
{
    if (zoom_punch.active)
    {
        zoom = zoom_punch.base_zoom;
    }
    zoom_punch.active = false;
    zoom_punch.elapsed = 0.0f;
}

void View2D::update_trauma(float dt)
{
    if (trauma_shake.trauma <= 0.0f)
    {
        return;
    }

    trauma_shake.time += dt;
    float t = trauma_shake.trauma * trauma_shake.trauma;

    float wave_x = std::sin(trauma_shake.time * trauma_shake.frequency * 2.0f * M_PI_F);
    float wave_y = std::cos((trauma_shake.time * trauma_shake.frequency + 0.37f) * 2.0f * M_PI_F);

    shake_offset.x += wave_x * trauma_shake.amplitudeX * t;
    shake_offset.y += wave_y * trauma_shake.amplitudeY * t;

    trauma_shake.trauma -= trauma_shake.decay * dt;
    if (trauma_shake.trauma < 0.0f)
    {
        trauma_shake.trauma = 0.0f;
    }
}

void View2D::update_zoom_punch(float dt)
{
    if (!zoom_punch.active)
    {
        return;
    }

    zoom_punch.elapsed += dt;
    float t = zoom_punch.elapsed / zoom_punch.duration;
    if (t >= 1.0f)
    {
        zoom = zoom_punch.base_zoom;
        zoom_punch.active = false;
        return;
    }

    float pulse = std::sin(t * M_PI_F);
    zoom = zoom_punch.base_zoom + zoom_punch.amount * pulse;
}

// ----------------------------------------------------------
// _update  —  smooth follow + dead zone
// ----------------------------------------------------------

void View2D::_update(float dt)
{
    shake_offset = Vec2(0.0f, 0.0f);
    update_shake(dt);
    update_trauma(dt);
    update_zoom_punch(dt);

    if (!follow_target) return;

    Vec2 target_pos = follow_target->get_global_position();
    Vec2 cam_pos    = get_global_position();
    Vec2 delta      = target_pos - cam_pos;

    // Dead zone: only move if target exits the box
    if (dead_zone.x > 0.0f || dead_zone.y > 0.0f)
    {
        if (std::fabs(delta.x) < dead_zone.x) delta.x = 0.0f;
        if (std::fabs(delta.y) < dead_zone.y) delta.y = 0.0f;
    }

    // Lerp toward target
    float t      = 1.0f - std::pow(1.0f - std::clamp(follow_speed * dt, 0.0f, 1.0f), 1.0f);
    position    += delta * t;
}
