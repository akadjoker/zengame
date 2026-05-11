
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
        m_cached_world = Matrix2DMult(parent2d->get_global_transform(), local);
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
