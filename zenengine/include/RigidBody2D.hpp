#pragma once

#include "CollisionObject2D.hpp"
#include <functional>

// ----------------------------------------------------------------------------
// RigidBody2D
//
// Dynamic physics body with velocity, gravity and simple collision response.
// Integrates Euler physics and resolves collisions against:
//   - StaticBody2D / CollisionObject2D nodes (via SpatialHash)
//   - TileMap2D solid cells
//
// Usage:
//   auto* body = tree.create<RigidBody2D>(root, "Ball");
//   body->linear_velocity = Vec2(200, -400);
//   body->gravity_scale   = 1.0f;
//   body->bounciness      = 0.6f;
//   body->on_body_entered = [&](CollisionObject2D* other) { ... };
// ----------------------------------------------------------------------------
class RigidBody2D : public CollisionObject2D
{
public:
    explicit RigidBody2D(const std::string& p_name = "RigidBody2D");

    // ── Physics properties ────────────────────────────────────────────────────
    Vec2  linear_velocity  = {0.0f, 0.0f};
    float angular_velocity = 0.0f;        // degrees per second
    float mass             = 1.0f;
    float gravity_scale    = 1.0f;        // 0 = no gravity
    float linear_damping   = 0.0f;        // 0..1 per second
    float angular_damping  = 0.05f;
    float bounciness       = 0.2f;        // 0 = no bounce, 1 = perfect bounce
    float friction         = 0.1f;        // applied on contact tangent

    // ── Control flags ─────────────────────────────────────────────────────────
    bool  freeze_position  = false;   // no positional integration
    bool  freeze_rotation  = false;   // no rotational integration
    bool  collide_with_tilemap = true;

    // ── Forces ────────────────────────────────────────────────────────────────
    void apply_force   (Vec2 force);            // continuous (per-frame)
    void apply_impulse (Vec2 impulse);          // instant velocity change
    void apply_torque  (float torque_deg_s2);   // continuous angular force

    // ── State ─────────────────────────────────────────────────────────────────
    bool  is_on_floor  () const { return m_on_floor; }
    bool  is_on_wall   () const { return m_on_wall;  }
    bool  is_on_ceiling() const { return m_on_ceil;  }

    // ── Callbacks ─────────────────────────────────────────────────────────────
    std::function<void(CollisionObject2D*)> on_body_entered;
    std::function<void()>                   on_floor_landed;

    // ── Node2D override ───────────────────────────────────────────────────────
    void _update(float dt) override;

private:
    Vec2  m_accum_force  = {};   // accumulated force this frame
    float m_accum_torque = 0.0f;

    bool  m_on_floor    = false;
    bool  m_on_wall     = false;
    bool  m_on_ceil     = false;
    bool  m_was_on_floor = false;

    static constexpr float GRAVITY = 980.0f;  // pixels/s²

    void integrate     (float dt);
    void resolve_bodies(float dt);
    void resolve_tiles (float dt);
};
