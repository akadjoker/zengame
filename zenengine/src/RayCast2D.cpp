#include "RayCast2D.hpp"
#include "CollisionObject2D.hpp"
#include "Collider2D.hpp"
#include "TileMap2D.hpp"
#include "SceneTree.hpp"
#include "Node.hpp"
#include <raylib.h>
#include <cmath>
#include <cfloat>
#include <vector>

// ----------------------------------------------------------------------------
// Ray-vs-AABB slab test
// Returns true if the infinite ray (origin, dir) intersects the AABB.
// out_t    = distance along the ray [0..1] relative to (cast_to - origin)
// out_normal = surface normal at the hit face
// ----------------------------------------------------------------------------
bool RayCast2D::ray_vs_aabb(Vec2 origin, Vec2 dir,
                             Rectangle aabb,
                             float& out_t, Vec2& out_normal)
{
    const float min_x = aabb.x;
    const float min_y = aabb.y;
    const float max_x = aabb.x + aabb.width;
    const float max_y = aabb.y + aabb.height;

    float t_near_x = (dir.x != 0.0f) ? (min_x - origin.x) / dir.x : -FLT_MAX;
    float t_far_x  = (dir.x != 0.0f) ? (max_x - origin.x) / dir.x :  FLT_MAX;
    float t_near_y = (dir.y != 0.0f) ? (min_y - origin.y) / dir.y : -FLT_MAX;
    float t_far_y  = (dir.y != 0.0f) ? (max_y - origin.y) / dir.y :  FLT_MAX;

    if (t_near_x > t_far_x) { float tmp = t_near_x; t_near_x = t_far_x; t_far_x = tmp; }
    if (t_near_y > t_far_y) { float tmp = t_near_y; t_near_y = t_far_y; t_far_y = tmp; }

    const float t_enter = (t_near_x > t_near_y) ? t_near_x : t_near_y;
    const float t_exit  = (t_far_x  < t_far_y)  ? t_far_x  : t_far_y;

    if (t_exit < 0.0f || t_enter > t_exit || t_enter > 1.0f || t_exit < 0.0f)
        return false;

    out_t = t_enter;

    // Determine hit normal from which axis gave the entry
    if (t_near_x > t_near_y)
        out_normal = (dir.x < 0.0f) ? Vec2{1.0f, 0.0f} : Vec2{-1.0f, 0.0f};
    else
        out_normal = (dir.y < 0.0f) ? Vec2{0.0f, 1.0f} : Vec2{0.0f, -1.0f};

    return true;
}

// ----------------------------------------------------------------------------
// Helpers — collect TileMaps (same pattern as CharacterBody2D)
// ----------------------------------------------------------------------------
static void CollectTileMaps(Node* node, std::vector<TileMap2D*>& out)
{
    if (!node) return;
    if (auto* tm = dynamic_cast<TileMap2D*>(node)) out.push_back(tm);
    for (size_t i = 0; i < node->get_child_count(); ++i)
        CollectTileMaps(node->get_child(i), out);
}

// ----------------------------------------------------------------------------
// RayCast2D
// ----------------------------------------------------------------------------
RayCast2D::RayCast2D(const std::string& p_name)
    : Node2D(p_name)
{
    node_type = NodeType::RayCast2D;
}

void RayCast2D::force_update()
{
    do_cast();
}

void RayCast2D::_update(float dt)
{
    Node2D::_update(dt);
    if (enabled) do_cast();
}

// ----------------------------------------------------------------------------
void RayCast2D::do_cast()
{
    // Reset results
    m_hit          = false;
    m_hit_point    = {};
    m_hit_normal   = {};
    m_hit_body     = nullptr;
    m_hit_fraction = 1.0f;

    if (!m_tree) return;

    // World-space origin and direction vector
    const Vec2  origin = get_global_position();
    const Vec2  end    = get_global_transform().TransformCoords(cast_to);
    const Vec2  dir    = { end.x - origin.x, end.y - origin.y };
    const float len    = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.001f) return;

    float best_t = 1.0f;

    // ── Broad-phase AABB covering the ray segment ──────────────────────────
    const float ray_min_x = (origin.x < end.x) ? origin.x : end.x;
    const float ray_min_y = (origin.y < end.y) ? origin.y : end.y;
    const float ray_max_x = (origin.x > end.x) ? origin.x : end.x;
    const float ray_max_y = (origin.y > end.y) ? origin.y : end.y;
    const Rectangle ray_aabb { ray_min_x, ray_min_y,
                                ray_max_x - ray_min_x,
                                ray_max_y - ray_min_y };

    // ── Check CollisionObject2D bodies ────────────────────────────────────
    if (collide_with_bodies)
    {
        std::vector<CollisionObject2D*> candidates;
        m_tree->query_collision_candidates(ray_aabb, nullptr, candidates);

        for (CollisionObject2D* body : candidates)
        {
            if (!body || !body->enabled) continue;
            if (!(body->collision_layer & collision_mask)) continue;

            std::vector<Collider2D*> colliders;
            body->get_colliders(colliders);

            for (Collider2D* col : colliders)
            {
                if (!col) continue;
                const Rectangle aabb = col->get_world_aabb();

                float t = 1.0f;
                Vec2  n = {};
                if (ray_vs_aabb(origin, dir, aabb, t, n) && t >= 0.0f && t < best_t)
                {
                    best_t          = t;
                    m_hit           = true;
                    m_hit_fraction  = t;
                    m_hit_point     = { origin.x + dir.x * t, origin.y + dir.y * t };
                    m_hit_normal    = n;
                    m_hit_body      = body;
                }
            }
        }
    }

    // ── Check TileMap solid cells ─────────────────────────────────────────
    if (collide_with_tilemap && m_tree->get_root())
    {
        std::vector<TileMap2D*> maps;
        CollectTileMaps(m_tree->get_root(), maps);

        for (TileMap2D* tm : maps)
        {
            if (!tm || !tm->visible || tm->width <= 0 || tm->height <= 0) continue;

            // World-space grid range for the ray AABB
            int gx0, gy0, gx1, gy1;
            tm->world_to_grid(Vec2(ray_min_x, ray_min_y), gx0, gy0);
            tm->world_to_grid(Vec2(ray_max_x, ray_max_y), gx1, gy1);
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

                    float t = 1.0f;
                    Vec2  n = {};
                    if (ray_vs_aabb(origin, dir, cell, t, n) && t >= 0.0f && t < best_t)
                    {
                        best_t         = t;
                        m_hit          = true;
                        m_hit_fraction = t;
                        m_hit_point    = { origin.x + dir.x * t, origin.y + dir.y * t };
                        m_hit_normal   = n;
                        m_hit_body     = nullptr;  // tilemap hit — no body
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Debug draw
// ----------------------------------------------------------------------------
void RayCast2D::_draw()
{
    Node2D::_draw();
    if (!show_debug) return;

    const Vec2 origin = get_global_position();
    const Vec2 end    = get_global_transform().TransformCoords(cast_to);

    if (m_hit)
    {
        // Green up to hit point, red beyond
        DrawLineEx({origin.x, origin.y}, {m_hit_point.x, m_hit_point.y},
                   2.0f, Color{0, 220, 80, 255});
        DrawLineEx({m_hit_point.x, m_hit_point.y}, {end.x, end.y},
                   1.0f, Color{220, 60, 60, 180});
        // Normal
        DrawLineEx({m_hit_point.x, m_hit_point.y},
                   {m_hit_point.x + m_hit_normal.x * 16.0f,
                    m_hit_point.y + m_hit_normal.y * 16.0f},
                   2.0f, YELLOW);
        DrawCircleV({m_hit_point.x, m_hit_point.y}, 4.0f, Color{255, 255, 0, 220});
    }
    else
    {
        DrawLineEx({origin.x, origin.y}, {end.x, end.y},
                   1.5f, Color{0, 160, 255, 160});
    }
}
