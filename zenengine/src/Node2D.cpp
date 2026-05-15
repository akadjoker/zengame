
#include "pch.hpp"
#include "Node2D.hpp"

// ============================================================
// Node2D - Implementation
// ============================================================

uint32_t Node2D::s_current_frame = 0;

void Node2D::begin_frame(uint32_t frame_id)
{
    s_current_frame = frame_id;
}

// ----------------------------------------------------------
// Constructor
// ----------------------------------------------------------

Node2D::Node2D(const std::string& p_name)
    : Node(p_name)
    , position(0.0f, 0.0f)
    , scale(1.0f, 1.0f)
    , pivot(0.0f, 0.0f)
    , skew(0.0f, 0.0f)
    , rotation(0.0f)
    , z_index(0)
{
    node_type = NodeType::Node2D;
}

// ----------------------------------------------------------
// Transform cache invalidation
// ----------------------------------------------------------

void Node2D::invalidate_transform()
{
    // Bump the cache stamp to force recomputation for this node and children
    m_cache_frame = 0xFFFFFFFFu;

    for (Node* child : m_children)
    {
        if (child->is_node2d())
        {
            static_cast<Node2D*>(child)->invalidate_transform();
        }
    }
}

// ----------------------------------------------------------
// Transform
// ----------------------------------------------------------

Matrix2D Node2D::get_local_transform() const
{
    return GetRelativeTransformation(
        position.x, position.y,
        scale.x,    scale.y,
        skew.x,     skew.y,
        pivot.x,    pivot.y,
        rotation
    );
}

Matrix2D Node2D::get_global_transform() const
{
    if (m_cache_frame == s_current_frame)
    {
        return m_cached_world;
    }

    Matrix2D local = get_local_transform();

    if (m_parent && m_parent->is_node2d())
    {
        const Node2D* parent2d = static_cast<const Node2D*>(m_parent);
        // Convention: Matrix2DMult(A,B) = "apply A first, then B".
        // To go from local space to world space we need: local first, then parent.
        m_cached_world = Matrix2DMult(local, parent2d->get_global_transform());
    }
    else
    {
        m_cached_world = local;
    }

    m_cache_frame = s_current_frame;
    return m_cached_world;
}

Vec2 Node2D::get_global_position() const
{
    Matrix2D g = get_global_transform();
    return Vec2(g.tx, g.ty);
}

float Node2D::get_global_rotation() const
{
    float total = rotation;

    const Node* current = m_parent;
    while (current)
    {
        if (current->is_node2d())
        {
            total += static_cast<const Node2D*>(current)->rotation;
        }
        current = current->get_parent();
    }

    return total;
}

Vec2 Node2D::local_to_world(const Vec2 &local_point) const
{
    return get_global_transform().TransformCoords(local_point);
}

Vec2 Node2D::world_to_local(const Vec2 &world_point) const
{
    return get_global_transform().Invert().TransformCoords(world_point);
}

void Node2D::translate(const Vec2& delta)
{
    position += delta;
}

void Node2D::rotate_by(float degrees)
{
    rotation += degrees;
}

void Node2D::advance(float dt)
{
    const float rad = rotation * DEG_TO_RAD;
    position.x += std::cos(rad) * dt;
    position.y += std::sin(rad) * dt;
}

void Node2D::advance(float dt, float angle)
{
    const float rad = angle * DEG_TO_RAD;
    position.x += std::cos(rad) * dt;
    position.y += std::sin(rad) * dt;
}

void Node2D::look_at(const Vec2& target)
{
    Vec2 global_pos = get_global_position();
    Vec2 dir        = target - global_pos;
    rotation = std::atan2(dir.y, dir.x) * RAD_TO_DEG;
}

Vec2 Node2D::to_global(const Vec2& local_point) const
{
    return get_global_transform().TransformCoords(local_point);
}

Vec2 Node2D::to_local(const Vec2& world_point) const
{
    return world_to_local(world_point);
}

void Node2D::set_z_index(int value)
{
    if (z_index == value)
    {
        return;
    }

    z_index = value;
    if (m_parent)
    {
        m_parent->mark_children_draw_order_dirty();
    }
}

int Node2D::get_z_index() const
{
    return z_index;
}

int Node2D::get_draw_order() const
{
    return z_index;
}

// ----------------------------------------------------------
// Helpers
// ----------------------------------------------------------

float Node2D::distance_to(const Vec2& world_point) const
{
    return get_global_position().distance(world_point);
}

float Node2D::distance_to(const Node2D* other) const
{
    if (!other) return 0.0f;
    return get_global_position().distance(other->get_global_position());
}

Vec2 Node2D::direction_to(const Vec2& world_point) const
{
    return (world_point - get_global_position()).normalised();
}

Vec2 Node2D::direction_to(const Node2D* other) const
{
    if (!other) return Vec2(0.0f, 0.0f);
    return direction_to(other->get_global_position());
}

Vec2 Node2D::move_toward(const Vec2& target, float max_distance)
{
    const Vec2  diff = target - position;
    const float dist = diff.magnitude();
    if (dist <= max_distance || dist < 1e-6f)
        position = target;
    else
        position = position + diff * (max_distance / dist);
    invalidate_transform();
    return position;
}

float Node2D::rotate_toward(float target_deg, float max_delta)
{
    float diff = target_deg - rotation;
    // wrap to [-180, 180]
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    if (fabsf(diff) <= max_delta)
        rotation = target_deg;
    else
        rotation += (diff > 0.0f ? max_delta : -max_delta);
    return rotation;
}

void Node2D::set_global_position(const Vec2& world_pos)
{
    if (m_parent && m_parent->is_node2d())
        position = static_cast<Node2D*>(m_parent)->world_to_local(world_pos);
    else
        position = world_pos;
    invalidate_transform();
}

// ── Immediate-mode draw helpers ───────────────────────────────────────────────

void Node2D::draw_circle(Vec2 local_pos, float radius, Color c, bool filled) const
{
    const Vec2 w = to_global(local_pos);
    if (filled)
        DrawCircleV({w.x, w.y}, radius, c);
    else
        DrawCircleLinesV({w.x, w.y}, radius, c);
}

void Node2D::draw_rect(Vec2 local_origin, Vec2 size, Color c, bool filled) const
{
    // Transform all four corners then draw as a polygon so rotation is respected.
    const Vec2 tl = to_global(local_origin);
    const Vec2 tr = to_global({local_origin.x + size.x, local_origin.y});
    const Vec2 br = to_global({local_origin.x + size.x, local_origin.y + size.y});
    const Vec2 bl = to_global({local_origin.x,           local_origin.y + size.y});

    if (filled)
    {
        // Two triangles
        DrawTriangle({tl.x, tl.y}, {bl.x, bl.y}, {br.x, br.y}, c);
        DrawTriangle({tl.x, tl.y}, {br.x, br.y}, {tr.x, tr.y}, c);
    }
    else
    {
        DrawLineEx({tl.x, tl.y}, {tr.x, tr.y}, 1.0f, c);
        DrawLineEx({tr.x, tr.y}, {br.x, br.y}, 1.0f, c);
        DrawLineEx({br.x, br.y}, {bl.x, bl.y}, 1.0f, c);
        DrawLineEx({bl.x, bl.y}, {tl.x, tl.y}, 1.0f, c);
    }
}

void Node2D::draw_line(Vec2 local_a, Vec2 local_b, Color c, float width) const
{
    const Vec2 a = to_global(local_a);
    const Vec2 b = to_global(local_b);
    DrawLineEx({a.x, a.y}, {b.x, b.y}, width, c);
}

void Node2D::draw_triangle(Vec2 a, Vec2 b, Vec2 c_pt, Color c, bool filled) const
{
    const Vec2 wa = to_global(a);
    const Vec2 wb = to_global(b);
    const Vec2 wc = to_global(c_pt);
    if (filled)
        DrawTriangle({wa.x, wa.y}, {wb.x, wb.y}, {wc.x, wc.y}, c);
    else
        DrawTriangleLines({wa.x, wa.y}, {wb.x, wb.y}, {wc.x, wc.y}, c);
}
