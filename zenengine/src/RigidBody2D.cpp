#include "RigidBody2D.hpp"
#include "Collider2D.hpp"
#include "CollisionObject2D.hpp"
#include "StaticBody2D.hpp"
#include "TileMap2D.hpp"
#include "SceneTree.hpp"
#include "Collision2D.hpp"
#include <raylib.h>
#include <cmath>
#include <vector>
#include <algorithm>

// ----------------------------------------------------------------------------
// Helpers (same pattern as CharacterBody2D)
// ----------------------------------------------------------------------------
namespace
{

static void CollectTileMaps(Node* node, std::vector<TileMap2D*>& out)
{
    if (!node) return;
    if (auto* tm = dynamic_cast<TileMap2D*>(node)) out.push_back(tm);
    for (size_t i = 0; i < node->get_child_count(); ++i)
        CollectTileMaps(node->get_child(i), out);
}

// AABB overlap + penetration depth along each axis
static bool aabb_overlap(Rectangle a, Rectangle b, Vec2& pen)
{
    const float dx = (a.x + a.width  * 0.5f) - (b.x + b.width  * 0.5f);
    const float dy = (a.y + a.height * 0.5f) - (b.y + b.height * 0.5f);
    const float ox = (a.width  + b.width)  * 0.5f - fabsf(dx);
    const float oy = (a.height + b.height) * 0.5f - fabsf(dy);
    if (ox <= 0.0f || oy <= 0.0f) return false;
    if (ox < oy)
        pen = { (dx >= 0.0f ? ox : -ox), 0.0f };
    else
        pen = { 0.0f, (dy >= 0.0f ? oy : -oy) };
    return true;
}

} // namespace

// ----------------------------------------------------------------------------
// RigidBody2D
// ----------------------------------------------------------------------------

RigidBody2D::RigidBody2D(const std::string& p_name)
    : CollisionObject2D(p_name)
{
    is_trigger = false;
}

// ── Forces ────────────────────────────────────────────────────────────────────

void RigidBody2D::apply_force(Vec2 force)
{
    m_accum_force.x += force.x;
    m_accum_force.y += force.y;
}

void RigidBody2D::apply_impulse(Vec2 impulse)
{
    if (mass > 0.0f)
    {
        linear_velocity.x += impulse.x / mass;
        linear_velocity.y += impulse.y / mass;
    }
}

void RigidBody2D::apply_torque(float torque_deg_s2)
{
    m_accum_torque += torque_deg_s2;
}

// ── Update ────────────────────────────────────────────────────────────────────

void RigidBody2D::_update(float dt)
{
    Node2D::_update(dt);
    if (!enabled) return;

    m_was_on_floor = m_on_floor;
    m_on_floor = m_on_wall = m_on_ceil = false;

    integrate(dt);
    resolve_tiles(dt);
    resolve_bodies(dt);

    // Fire floor_landed on transition
    if (m_on_floor && !m_was_on_floor && on_floor_landed)
        on_floor_landed();

    // Reset per-frame accumulations
    m_accum_force  = {};
    m_accum_torque = 0.0f;
}

// ── Physics integration ───────────────────────────────────────────────────────

void RigidBody2D::integrate(float dt)
{
    if (mass <= 0.0f) return;

    // Gravity
    if (gravity_scale != 0.0f)
        m_accum_force.y += GRAVITY * gravity_scale * mass;

    // Acceleration = F / m
    linear_velocity.x += (m_accum_force.x / mass) * dt;
    linear_velocity.y += (m_accum_force.y / mass) * dt;
    angular_velocity  += (m_accum_torque / mass)  * dt;

    // Damping
    const float ld = 1.0f - std::min(linear_damping  * dt, 1.0f);
    const float ad = 1.0f - std::min(angular_damping * dt, 1.0f);
    linear_velocity.x *= ld;
    linear_velocity.y *= ld;
    angular_velocity  *= ad;

    // Integrate position
    if (!freeze_position)
    {
        position.x += linear_velocity.x * dt;
        position.y += linear_velocity.y * dt;
    }
    if (!freeze_rotation)
    {
        rotation += angular_velocity * dt;
    }
}

// ── Tile collision ────────────────────────────────────────────────────────────

void RigidBody2D::resolve_tiles(float /*dt*/)
{
    if (!collide_with_tilemap || !m_tree) return;

    Collider2D* col = get_collider();
    if (!col) return;

    Rectangle body_aabb = col->get_world_aabb();

    std::vector<TileMap2D*> maps;
    CollectTileMaps(m_tree->get_root(), maps);

    for (TileMap2D* tm : maps)
    {
        if (!tm || !tm->visible || tm->width <= 0 || tm->height <= 0) continue;

        int gx0, gy0, gx1, gy1;
        tm->world_to_grid(Vec2(body_aabb.x,                 body_aabb.y),                 gx0, gy0);
        tm->world_to_grid(Vec2(body_aabb.x + body_aabb.width, body_aabb.y + body_aabb.height), gx1, gy1);
        gx0 = std::max(0, std::min(gx0, tm->width  - 1));
        gy0 = std::max(0, std::min(gy0, tm->height - 1));
        gx1 = std::max(0, std::min(gx1, tm->width  - 1));
        gy1 = std::max(0, std::min(gy1, tm->height - 1));
        if (gx1 < gx0) std::swap(gx0, gx1);
        if (gy1 < gy0) std::swap(gy0, gy1);

        for (int gy = gy0; gy <= gy1; ++gy)
        {
            for (int gx = gx0; gx <= gx1; ++gx)
            {
                if (!tm->is_solid_cell(gx, gy)) continue;
                const Rectangle cell = tm->cell_rect_world(gx, gy);

                Vec2 pen = {};
                if (!aabb_overlap(body_aabb, cell, pen)) continue;

                // Resolve penetration
                if (!freeze_position)
                {
                    position.x += pen.x;
                    position.y += pen.y;
                    body_aabb.x += pen.x;
                    body_aabb.y += pen.y;
                }

                // Velocity response along the collision normal
                const Vec2 normal = {
                    pen.x != 0.0f ? (pen.x > 0.0f ? 1.0f : -1.0f) : 0.0f,
                    pen.y != 0.0f ? (pen.y > 0.0f ? 1.0f : -1.0f) : 0.0f
                };

                const float vdot = linear_velocity.x * normal.x +
                                   linear_velocity.y * normal.y;
                if (vdot < 0.0f)  // moving into the surface
                {
                    // Reflect with bounciness
                    linear_velocity.x -= (1.0f + bounciness) * vdot * normal.x;
                    linear_velocity.y -= (1.0f + bounciness) * vdot * normal.y;

                    // Friction on tangent
                    const Vec2 tangent = { -normal.y, normal.x };
                    const float vtang = linear_velocity.x * tangent.x +
                                        linear_velocity.y * tangent.y;
                    linear_velocity.x -= friction * vtang * tangent.x;
                    linear_velocity.y -= friction * vtang * tangent.y;
                }

                // Floor/wall/ceil detection
                if (normal.y > 0.5f)  m_on_floor = true;
                if (normal.y < -0.5f) m_on_ceil  = true;
                if (fabsf(normal.x) > 0.5f) m_on_wall = true;
            }
        }
    }
}

// ── Body collision ────────────────────────────────────────────────────────────

void RigidBody2D::resolve_bodies(float /*dt*/)
{
    if (!m_tree) return;

    Collider2D* col = get_collider();
    if (!col) return;

    const Rectangle my_aabb = col->get_world_aabb();
    std::vector<CollisionObject2D*> candidates;
    m_tree->query_collision_candidates(my_aabb, this, candidates);

    for (CollisionObject2D* other : candidates)
    {
        if (!other || !other->enabled || other->is_trigger) continue;
        if (!Collision2D::CanCollide(collision_layer, collision_mask, other->collision_layer, other->collision_mask)) continue;

        std::vector<Collider2D*> other_cols;
        other->get_colliders(other_cols);

        for (Collider2D* oc : other_cols)
        {
            if (!oc) continue;
            Contact2D contact;
            if (!Collision2D::Collide(*col, *oc, &contact) || contact.depth <= 0.0f)
            {
                continue;
            }

            Vec2 normal = contact.normal;
            const Vec2 this_center = col->get_world_center();
            const Vec2 other_center = oc->get_world_center();
            if ((this_center - other_center).dot(normal) < 0.0f)
            {
                normal = -normal;
            }

            // Resolve
            if (!freeze_position)
            {
                position += normal * contact.depth;
            }

            const float vdot = linear_velocity.x * normal.x +
                               linear_velocity.y * normal.y;
            if (vdot < 0.0f)
            {
                linear_velocity.x -= (1.0f + bounciness) * vdot * normal.x;
                linear_velocity.y -= (1.0f + bounciness) * vdot * normal.y;
            }

            if (normal.y > 0.5f)  m_on_floor = true;
            if (normal.y < -0.5f) m_on_ceil  = true;
            if (fabsf(normal.x) > 0.5f) m_on_wall = true;

            if (on_body_entered) on_body_entered(other);
        }
    }
}
