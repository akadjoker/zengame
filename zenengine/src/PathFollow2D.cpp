#include "PathFollow2D.hpp"
#include "Path2D.hpp"
#include "Node.hpp"

// ── Constructor ───────────────────────────────────────────────────────────────
PathFollow2D::PathFollow2D(const std::string& p_name) : Node2D(p_name)
{
    node_type = NodeType::PathFollow2D;
}

// ── Internal helper ───────────────────────────────────────────────────────────
Path2D* PathFollow2D::get_path() const
{
    if (m_parent && m_parent->node_type == NodeType::Path2D)
        return static_cast<Path2D*>(m_parent);
    return nullptr;
}

// ── Normalised progress ───────────────────────────────────────────────────────
float PathFollow2D::get_progress() const
{
    Path2D* path = get_path();
    if (!path) return 0.0f;
    const float total = path->get_total_length();
    return (total > 0.0f) ? offset / total : 0.0f;
}

void PathFollow2D::set_progress(float t)
{
    Path2D* path = get_path();
    if (!path) return;
    offset = t * path->get_total_length();
}

// ── Update ────────────────────────────────────────────────────────────────────
void PathFollow2D::_update(float dt)
{
    Node2D::_update(dt);

    Path2D* path = get_path();
    if (!path) return;

    const float total = path->get_total_length();
    if (total <= 0.0f) return;

    // ── Advance offset ──────────────────────────────────────────────────────
    if (speed != 0.0f)
    {
        offset += speed * dt;

        if (loop)
        {
            if (offset >= total || offset < 0.0f)
            {
                offset = fmodf(offset, total);
                if (offset < 0.0f) offset += total;
                if (on_loop_completed) on_loop_completed();
            }
        }
        else
        {
            if (speed > 0.0f && offset >= total)
            {
                offset = total;
                speed  = 0.0f;   // stop
                if (on_reached_end) on_reached_end();
            }
            else if (speed < 0.0f && offset <= 0.0f)
            {
                offset = 0.0f;
                speed  = 0.0f;
                if (on_reached_end) on_reached_end();
            }
        }
    }
    else if (loop && total > 0.0f)
    {
        // Manual mode: keep offset clamped / wrapped
        offset = fmodf(offset, total);
        if (offset < 0.0f) offset += total;
    }
    else
    {
        offset = std::max(0.0f, std::min(offset, total));
    }

    // ── Query path for world position and direction ─────────────────────────
    const Vec2 world_pos = path->get_point_at_distance(offset);
    const Vec2 dir       = path->get_direction_at_distance(offset);

    // Apply h_offset (lateral shift in screen space)
    Vec2 final_pos = world_pos;
    if (h_offset != 0.0f)
    {
        const Vec2 perp(-dir.y, dir.x);
        final_pos.x += perp.x * h_offset;
        final_pos.y += perp.y * h_offset;
    }

    // Convert world position to local (relative to Path2D's parent space).
    // PathFollow2D is a child of Path2D, so set_global_position is correct.
    set_global_position(final_pos);   // also calls invalidate_transform()

    // ── Align rotation to tangent ───────────────────────────────────────────
    if (rotate)
    {
        rotation = atan2f(dir.y, dir.x) * (180.0f / 3.14159265f);
        invalidate_transform();  // rotation changed manually — bust cache
    }
}
