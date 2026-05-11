#pragma once

#include "Node2D.hpp"
#include <raylib.h>

class CollisionObject2D;

// ----------------------------------------------------------------------------
// RayCast2D
//
// Casts a ray from the node's world position toward (position + cast_to).
// Checks against CollisionObject2D nodes in the SceneTree (via SpatialHash)
// and optionally against TileMap2D solid cells.
//
// The ray is updated every frame if enabled, or on demand via force_update().
//
// Usage:
//   auto* ray = tree.create<RayCast2D>(player, "Sight");
//   ray->cast_to    = Vec2(200, 0);   // 200px to the right
//   ray->show_debug = true;
//   // in _update:
//   if (ray->is_colliding())
//       TraceLog(LOG_INFO, "Hit: %s", ray->get_collider()->get_name().c_str());
// ----------------------------------------------------------------------------
class RayCast2D : public Node2D
{
public:
    explicit RayCast2D(const std::string& p_name = "RayCast2D");

    // ── Configuration ─────────────────────────────────────────────────────────
    Vec2     cast_to              = {0.0f, 100.0f}; // end point (local space)
    bool     enabled              = true;
    bool     collide_with_bodies  = true;  // check CollisionObject2D nodes
    bool     collide_with_tilemap = true;  // check TileMap2D solid cells
    uint32_t collision_mask       = 0xFFFFFFFFu;
    bool     show_debug           = false;

    // ── Control ───────────────────────────────────────────────────────────────
    // Re-cast immediately (useful before reading results in the same frame).
    void force_update();

    // ── Results ───────────────────────────────────────────────────────────────
    bool                is_colliding         () const { return m_hit; }
    Vec2                get_collision_point  () const { return m_hit_point; }
    Vec2                get_collision_normal () const { return m_hit_normal; }
    CollisionObject2D*  get_collider         () const { return m_hit_body; }
    float               get_hit_fraction     () const { return m_hit_fraction; }

    // ── Node2D overrides ──────────────────────────────────────────────────────
    void _update(float dt) override;
    void _draw  ()         override;

private:
    // Hit results (cleared each cast)
    bool               m_hit          = false;
    Vec2               m_hit_point    = {};
    Vec2               m_hit_normal   = {};
    CollisionObject2D* m_hit_body     = nullptr;
    float              m_hit_fraction = 1.0f; // [0..1] along the ray

    void do_cast();

    // Ray-vs-AABB intersection; returns true and fills t, normal on hit.
    static bool ray_vs_aabb(Vec2 origin, Vec2 dir, Rectangle aabb,
                             float& out_t, Vec2& out_normal);
};
