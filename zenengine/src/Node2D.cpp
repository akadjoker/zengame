
#include "pch.hpp"
#include "Node2D.hpp"

// ============================================================
// Node2D - Implementation
// ============================================================

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
}

// ----------------------------------------------------------
// Transform
// ----------------------------------------------------------

Matrix2D Node2D::get_local_transform() const
{
    // Builds the full local matrix using position, rotation, scale and pivot
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
    Matrix2D local = get_local_transform();

    // Walk up the parent chain and concatenate transforms
    if (m_parent)
    {
        // Only Node2D parents contribute a spatial transform
        const Node2D* parent2d = dynamic_cast<const Node2D*>(m_parent);

        if (parent2d)
        {
            Matrix2D parent_global = parent2d->get_global_transform();
            // global = parent_global * local
            return Matrix2DMult(parent_global, local);
        }
    }

    return local;
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
        const Node2D* parent2d = dynamic_cast<const Node2D*>(current);
        if (parent2d)
        {
            total += parent2d->rotation;
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
    position.x += cos(rotation * DEG_TO_RAD) * dt;
    position.y += sin(rotation * DEG_TO_RAD) * dt;

}

void Node2D::advance(float dt, float angle)
{
    position.x += cos(angle * DEG_TO_RAD) * dt;
    position.y += sin(angle * DEG_TO_RAD) * dt;
}

void Node2D::look_at(const Vec2& target)
{
    Vec2 global_pos = get_global_position();
    Vec2 dir        = target - global_pos;

    // atan2 returns radians; convert to degrees
    rotation = std::atan2(dir.y, dir.x) * RAD_TO_DEG;
}

Vec2 Node2D::to_global(const Vec2& local_point) const
{
    return get_global_transform().TransformCoords(local_point);
}

Vec2 Node2D::to_local(const Vec2& world_point) const
{
    // Invert the global transform to go from world -> local
    Matrix2D g = get_global_transform();

    // Inverse of a 2D affine matrix (no shear = simple formula)
    float det = g.a * g.d - g.b * g.c;

    if (std::fabs(det) < 1e-6f)
    {
        // Degenerate transform (zero scale) - return zero
        return Vec2(0.0f, 0.0f);
    }

    float inv_det = 1.0f / det;

    Matrix2D inv;
    inv.a  =  g.d * inv_det;
    inv.b  = -g.b * inv_det;
    inv.c  = -g.c * inv_det;
    inv.d  =  g.a * inv_det;
    inv.tx = (g.c * g.ty - g.d * g.tx) * inv_det;
    inv.ty = (g.b * g.tx - g.a * g.ty) * inv_det;

    return inv.TransformCoords(world_point);
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
