#include "RigidBody2D.hpp"
#include "Collider2D.hpp"
#include "CollisionObject2D.hpp"
#include "StaticBody2D.hpp"
#include "TileMap2D.hpp"
#include "SceneTree.hpp"
#include "Collision2D.hpp"

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
    node_type = NodeType::RigidBody2D;
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
    invalidate_transform();
}

// ── Inertia helper ────────────────────────────────────────────────────────────

static float compute_inertia(const Collider2D* col)
{
    // Rough moment of inertia for common shapes (about center, mass=1 implicit)
    if (!col) return 1.0f;
    switch (col->shape)
    {
    case Collider2D::ShapeType::Circle:
        return 0.5f * col->radius * col->radius; // I = 0.5 * m * r²
    case Collider2D::ShapeType::Rectangle:
        return (col->size.x * col->size.x + col->size.y * col->size.y) / 12.0f; // I = m*(w²+h²)/12
    default:
        // For polygon/segment, approximate with bounding radius
        {
            float max_r2 = 0.0f;
            for (const Vec2& p : col->points)
                max_r2 = std::max(max_r2, p.x * p.x + p.y * p.y);
            return max_r2 * 0.5f;
        }
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

                // Contact arm: from body center toward the colliding cell surface
                const Vec2 body_center = col->get_world_center();
                const Vec2 r = Vec2(
                    body_center.x - normal.x * (col->shape == Collider2D::ShapeType::Circle
                        ? col->radius : std::min(col->size.x, col->size.y) * 0.5f),
                    body_center.y - normal.y * (col->shape == Collider2D::ShapeType::Circle
                        ? col->radius : std::min(col->size.x, col->size.y) * 0.5f)
                ) - body_center;

                const float invM = (mass > 0.0f) ? 1.0f / mass : 0.0f;
                const float tI = (inertia > 0.0f) ? inertia : mass * compute_inertia(col);
                const float invI = (!freeze_rotation && tI > 0.0f) ? 1.0f / tI : 0.0f;

                // Velocity at contact point
                const float w = angular_velocity * (3.14159265f / 180.0f);
                const float vcx = linear_velocity.x + (-w * r.y);
                const float vcy = linear_velocity.y + ( w * r.x);

                const float vn = vcx * normal.x + vcy * normal.y;
                if (vn < 0.0f)
                {
                    const float rn = r.x * normal.y - r.y * normal.x;
                    const float kN = invM + invI * rn * rn;
                    const float nMass = (kN > 0.0f) ? 1.0f / kN : 0.0f;
                    const float jn = -(1.0f + bounciness) * vn * nMass;

                    linear_velocity.x += jn * normal.x * invM;
                    linear_velocity.y += jn * normal.y * invM;
                    if (invI > 0.0f)
                    {
                        const float crP = r.x * (jn * normal.y) - r.y * (jn * normal.x);
                        angular_velocity += (crP * invI) * (180.0f / 3.14159265f);
                    }

                    // Friction impulse
                    const Vec2 tangent = { -normal.y, normal.x };
                    const float w2 = angular_velocity * (3.14159265f / 180.0f);
                    const float vt = (linear_velocity.x + (-w2 * r.y)) * tangent.x
                                   + (linear_velocity.y + ( w2 * r.x)) * tangent.y;
                    const float rt = r.x * tangent.y - r.y * tangent.x;
                    const float kT = invM + invI * rt * rt;
                    const float tMass = (kT > 0.0f) ? 1.0f / kT : 0.0f;
                    float jt = -vt * tMass;
                    const float maxF = friction * jn;
                    jt = std::max(-maxF, std::min(jt, maxF));

                    linear_velocity.x += jt * tangent.x * invM;
                    linear_velocity.y += jt * tangent.y * invM;
                    if (invI > 0.0f)
                    {
                        const float crT = r.x * (jt * tangent.y) - r.y * (jt * tangent.x);
                        angular_velocity += (crT * invI) * (180.0f / 3.14159265f);
                    }
                }

                // Floor/wall/ceil detection
                if (normal.y > 0.5f)  m_on_floor = true;
                if (normal.y < -0.5f) m_on_ceil  = true;
                if (fabsf(normal.x) > 0.5f) m_on_wall = true;
            }
        }
    }
    invalidate_transform();
}

// ── Body collision ────────────────────────────────────────────────────────────

void RigidBody2D::resolve_bodies(float /*dt*/)
{
    if (!m_tree) return;

    Collider2D* col = get_collider();
    if (!col) return;

    // Auto-compute inertia for this body (I = mass * I_shape)
    const float I_a = (inertia > 0.0f) ? inertia : mass * compute_inertia(col);

    const Rectangle my_aabb = col->get_world_aabb();
    std::vector<CollisionObject2D*> candidates;
    m_tree->query_collision_candidates(my_aabb, this, candidates);

    for (CollisionObject2D* other : candidates)
    {
        if (!other || !other->enabled || other->is_trigger) continue;
        if (!Collision2D::CanCollide(collision_layer, collision_mask, other->collision_layer, other->collision_mask)) continue;

        // Detect if the other body is also a RigidBody2D
        RigidBody2D* other_rb = other->is_a(NodeType::RigidBody2D)
                                 ? static_cast<RigidBody2D*>(other) : nullptr;

        // For RigidBody2D pairs: only solve from the body with the lower address
        // to avoid applying the impulse twice (once from each body's _update)
        if (other_rb && this > other_rb) continue;

        std::vector<Collider2D*> other_cols;
        other->get_colliders(other_cols);

        for (Collider2D* oc : other_cols)
        {
            if (!oc) continue;
            Contact2D contact;
            if (!Collision2D::Collide(*col, *oc, &contact) || contact.depth <= 0.0f)
                continue;

            Vec2 normal = contact.normal;
            const Vec2 this_center  = col->get_world_center();
            const Vec2 other_center = oc->get_world_center();
            if ((this_center - other_center).dot(normal) < 0.0f)
                normal = -normal;
            // normal now points from other toward this

            // ── Inverse masses ─────────────────────────────────────────
            const float invM_a = (mass > 0.0f) ? 1.0f / mass : 0.0f;
            const float invI_a = (!freeze_rotation && I_a > 0.0f) ? 1.0f / I_a : 0.0f;

            const float I_b    = other_rb ? ((other_rb->inertia > 0.0f) ? other_rb->inertia
                                              : other_rb->mass * compute_inertia(oc)) : 0.0f;
            const float invM_b = (other_rb && other_rb->mass > 0.0f) ? 1.0f / other_rb->mass : 0.0f;
            const float invI_b = (other_rb && !other_rb->freeze_rotation && I_b > 0.0f) ? 1.0f / I_b : 0.0f;

            // ── Position resolution (split by mass) ────────────────────
            const float total_inv = invM_a + invM_b;
            if (total_inv > 0.0f)
            {
                if (!freeze_position)
                    position += normal * contact.depth * (invM_a / total_inv);
                if (other_rb && !other_rb->freeze_position)
                    other_rb->position -= normal * contact.depth * (invM_b / total_inv);
            }

            // ── Contact arm for body A: from A's center to contact surface ─
            float half_a;
            switch (col->shape)
            {
            case Collider2D::ShapeType::Circle:
                half_a = col->radius; break;
            case Collider2D::ShapeType::Rectangle:
                half_a = fabsf(normal.x) * col->size.x * 0.5f
                       + fabsf(normal.y) * col->size.y * 0.5f; break;
            default:
                half_a = std::max(contact.depth, 1.0f); break;
            }
            const Vec2 r_a = normal * (-half_a); // toward contact (−normal direction)

            // ── Contact arm for body B: from B's center to contact surface ─
            float half_b = 0.0f;
            if (other_rb)
            {
                switch (oc->shape)
                {
                case Collider2D::ShapeType::Circle:
                    half_b = oc->radius; break;
                case Collider2D::ShapeType::Rectangle:
                    half_b = fabsf(normal.x) * oc->size.x * 0.5f
                           + fabsf(normal.y) * oc->size.y * 0.5f; break;
                default:
                    half_b = std::max(contact.depth, 1.0f); break;
                }
            }
            const Vec2 r_b = normal * half_b; // toward contact (+normal direction from B)

            // ── Angular velocities in rad/s ────────────────────────────
            const float w_a = angular_velocity * (3.14159265f / 180.0f);
            const float w_b = other_rb ? other_rb->angular_velocity * (3.14159265f / 180.0f) : 0.0f;

            // Velocity at contact point: v + cross(w, r)  [Y-down: cross = (-w*r.y, w*r.x)]
            const float vcx_a = linear_velocity.x + (-w_a * r_a.y);
            const float vcy_a = linear_velocity.y + ( w_a * r_a.x);
            const float vcx_b = other_rb ? other_rb->linear_velocity.x + (-w_b * r_b.y) : 0.0f;
            const float vcy_b = other_rb ? other_rb->linear_velocity.y + ( w_b * r_b.x) : 0.0f;

            // ── Normal impulse ─────────────────────────────────────────
            const float vn = (vcx_a - vcx_b) * normal.x + (vcy_a - vcy_b) * normal.y;
            if (vn < 0.0f)
            {
                const float rn_a = r_a.x * normal.y - r_a.y * normal.x;
                const float rn_b = r_b.x * normal.y - r_b.y * normal.x;
                const float kN   = invM_a + invM_b + invI_a * rn_a * rn_a + invI_b * rn_b * rn_b;
                const float nMass = (kN > 0.0f) ? 1.0f / kN : 0.0f;

                const float e  = other_rb ? std::min(bounciness, other_rb->bounciness) : bounciness;
                const float jn = -(1.0f + e) * vn * nMass;

                // Apply to A (Y-down: += for angular)
                linear_velocity.x += jn * normal.x * invM_a;
                linear_velocity.y += jn * normal.y * invM_a;
                if (invI_a > 0.0f)
                {
                    const float crP_a = r_a.x * (jn * normal.y) - r_a.y * (jn * normal.x);
                    angular_velocity += (crP_a * invI_a) * (180.0f / 3.14159265f);
                }

                // Apply reaction to B
                if (other_rb)
                {
                    other_rb->linear_velocity.x -= jn * normal.x * invM_b;
                    other_rb->linear_velocity.y -= jn * normal.y * invM_b;
                    if (invI_b > 0.0f)
                    {
                        const float crP_b = r_b.x * (jn * normal.y) - r_b.y * (jn * normal.x);
                        other_rb->angular_velocity -= (crP_b * invI_b) * (180.0f / 3.14159265f);
                    }
                }

                // ── Friction impulse ───────────────────────────────────
                const Vec2 tangent = Vec2(-normal.y, normal.x);

                // Recompute velocities at contact after normal impulse
                const float w2_a  = angular_velocity * (3.14159265f / 180.0f);
                const float w2_b  = other_rb ? other_rb->angular_velocity * (3.14159265f / 180.0f) : 0.0f;
                const float vcx2_a = linear_velocity.x + (-w2_a * r_a.y);
                const float vcy2_a = linear_velocity.y + ( w2_a * r_a.x);
                const float vcx2_b = other_rb ? other_rb->linear_velocity.x + (-w2_b * r_b.y) : 0.0f;
                const float vcy2_b = other_rb ? other_rb->linear_velocity.y + ( w2_b * r_b.x) : 0.0f;
                const float vt     = (vcx2_a - vcx2_b) * tangent.x + (vcy2_a - vcy2_b) * tangent.y;

                const float rt_a   = r_a.x * tangent.y - r_a.y * tangent.x;
                const float rt_b   = r_b.x * tangent.y - r_b.y * tangent.x;
                const float kT     = invM_a + invM_b + invI_a * rt_a * rt_a + invI_b * rt_b * rt_b;
                const float tMass  = (kT > 0.0f) ? 1.0f / kT : 0.0f;

                float jt = -vt * tMass;
                const float fr    = other_rb ? sqrtf(friction * other_rb->friction) : friction;
                const float maxFr = fr * jn;
                jt = std::max(-maxFr, std::min(jt, maxFr));

                // Apply to A
                linear_velocity.x += jt * tangent.x * invM_a;
                linear_velocity.y += jt * tangent.y * invM_a;
                if (invI_a > 0.0f)
                {
                    const float crT_a = r_a.x * (jt * tangent.y) - r_a.y * (jt * tangent.x);
                    angular_velocity += (crT_a * invI_a) * (180.0f / 3.14159265f);
                }

                // Apply reaction to B
                if (other_rb)
                {
                    other_rb->linear_velocity.x -= jt * tangent.x * invM_b;
                    other_rb->linear_velocity.y -= jt * tangent.y * invM_b;
                    if (invI_b > 0.0f)
                    {
                        const float crT_b = r_b.x * (jt * tangent.y) - r_b.y * (jt * tangent.x);
                        other_rb->angular_velocity -= (crT_b * invI_b) * (180.0f / 3.14159265f);
                    }
                    other_rb->invalidate_transform();
                }
            }

            if (normal.y > 0.5f)  m_on_floor = true;
            if (normal.y < -0.5f) m_on_ceil  = true;
            if (fabsf(normal.x) > 0.5f) m_on_wall = true;

            if (on_body_entered) on_body_entered(other);
        }
    }
    invalidate_transform();
}
