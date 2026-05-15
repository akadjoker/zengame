#include "Path2D.hpp"

// ── Catmull-Rom helper ────────────────────────────────────────────────────────
static Vec2 catmull_rom(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    return Vec2(
        0.5f * (2*p1.x + (-p0.x+p2.x)*t + (2*p0.x-5*p1.x+4*p2.x-p3.x)*t2 + (-p0.x+3*p1.x-3*p2.x+p3.x)*t3),
        0.5f * (2*p1.y + (-p0.y+p2.y)*t + (2*p0.y-5*p1.y+4*p2.y-p3.y)*t2 + (-p0.y+3*p1.y-3*p2.y+p3.y)*t3)
    );
}

// ── Constructor ───────────────────────────────────────────────────────────────
Path2D::Path2D(const std::string& p_name) : Node2D(p_name)
{
    node_type = NodeType::Path2D;
}

// ── Point management ──────────────────────────────────────────────────────────
void Path2D::add_point(Vec2 local_pos, int at_index)
{
    if (at_index < 0 || at_index >= (int)points.size())
        points.push_back(local_pos);
    else
        points.insert(points.begin() + at_index, local_pos);
    m_dirty = true;
}

void Path2D::set_point(int index, Vec2 local_pos)
{
    if (index >= 0 && index < (int)points.size())
    {
        points[index] = local_pos;
        m_dirty = true;
    }
}

void Path2D::remove_point(int index)
{
    if (index >= 0 && index < (int)points.size())
    {
        points.erase(points.begin() + index);
        m_dirty = true;
    }
}

void Path2D::clear_points()
{
    points.clear();
    m_dirty = true;
}

// ── Spline sampling (local space) ─────────────────────────────────────────────
// t is in [0 .. N] for loop=true, or [0 .. N-1] for loop=false.
Vec2 Path2D::sample_local(float t) const
{
    const int n = (int)points.size();
    if (n == 0) return Vec2(0, 0);
    if (n == 1) return points[0];

    if (n == 2)
    {
        // Linear fallback for degenerate case
        const float lt = std::max(0.0f, std::min(t, 1.0f));
        return Vec2(points[0].x + (points[1].x - points[0].x) * lt,
                    points[0].y + (points[1].y - points[0].y) * lt);
    }

    if (loop)
    {
        t = fmodf(t, (float)n);
        if (t < 0.0f) t += (float)n;
        const int   seg = (int)t;
        const float lt  = t - (float)seg;
        const int i0 = (seg - 1 + n) % n;
        const int i1 = seg;
        const int i2 = (seg + 1) % n;
        const int i3 = (seg + 2) % n;
        return catmull_rom(points[i0], points[i1], points[i2], points[i3], lt);
    }
    else
    {
        t = std::max(0.0f, std::min(t, (float)(n - 1)));
        int seg = (int)t;
        if (seg >= n - 1) seg = n - 2;
        const float lt = t - (float)seg;
        const int i0 = std::max(0, seg - 1);
        const int i1 = seg;
        const int i2 = std::min(n - 1, seg + 1);
        const int i3 = std::min(n - 1, seg + 2);
        return catmull_rom(points[i0], points[i1], points[i2], points[i3], lt);
    }
}

// ── Arc-length LUT ────────────────────────────────────────────────────────────
void Path2D::bake() const
{
    m_lut.clear();
    m_total = 0.0f;

    const int n = (int)points.size();
    if (n < 2) { m_dirty = false; return; }

    const int   segs        = loop ? n : (n - 1);
    const int   total_steps = segs * BAKE_STEPS;
    const float t_end       = (float)segs;

    Vec2 prev = sample_local(0.0f);
    m_lut.push_back({0.0f, 0.0f});

    for (int i = 1; i <= total_steps; ++i)
    {
        const float t   = (float)i / (float)total_steps * t_end;
        const Vec2  cur = sample_local(t);
        const float dx  = cur.x - prev.x;
        const float dy  = cur.y - prev.y;
        m_total += sqrtf(dx * dx + dy * dy);
        m_lut.push_back({m_total, t});
        prev = cur;
    }
    m_dirty = false;
}

void Path2D::ensure_baked() const
{
    if (m_dirty) bake();
}

float Path2D::t_from_dist(float dist) const
{
    ensure_baked();
    if (m_lut.empty())     return 0.0f;
    if (dist <= 0.0f)      return m_lut.front().t;
    if (dist >= m_total)   return m_lut.back().t;

    // Binary search for the two LUT entries bracketing dist
    int lo = 0, hi = (int)m_lut.size() - 1;
    while (lo + 1 < hi)
    {
        const int mid = (lo + hi) / 2;
        if (m_lut[mid].arc <= dist) lo = mid; else hi = mid;
    }
    const float span = m_lut[hi].arc - m_lut[lo].arc;
    const float frac = (span > 1e-6f) ? (dist - m_lut[lo].arc) / span : 0.0f;
    return m_lut[lo].t + (m_lut[hi].t - m_lut[lo].t) * frac;
}

// ── Public arc-length queries ─────────────────────────────────────────────────
float Path2D::get_total_length() const
{
    ensure_baked();
    return m_total;
}

Vec2 Path2D::get_point_at_distance(float dist) const
{
    ensure_baked();
    if (loop && m_total > 0.0f)
        dist = fmodf(dist, m_total);
    return to_global(sample_local(t_from_dist(dist)));
}

Vec2 Path2D::get_direction_at_distance(float dist) const
{
    ensure_baked();
    if (m_total <= 0.0f) return Vec2(1, 0);
    if (loop)
        dist = fmodf(dist, m_total);

    const float eps = std::max(m_total * 0.001f, 0.5f);
    const Vec2 a = to_global(sample_local(t_from_dist(std::max(0.0f, dist - eps))));
    const Vec2 b = to_global(sample_local(t_from_dist(std::min(m_total, dist + eps))));
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len = sqrtf(dx * dx + dy * dy);
    return (len > 1e-6f) ? Vec2(dx / len, dy / len) : Vec2(1, 0);
}

float Path2D::get_closest_offset(Vec2 world_pos) const
{
    ensure_baked();
    if (m_lut.empty()) return 0.0f;

    const Vec2 local = to_local(world_pos);
    float best_dist2 = 1e30f;
    float best_arc   = 0.0f;

    for (const auto& entry : m_lut)
    {
        const Vec2  p  = sample_local(entry.t);
        const float dx = p.x - local.x;
        const float dy = p.y - local.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 < best_dist2) { best_dist2 = d2; best_arc = entry.arc; }
    }
    return best_arc;
}

// ── Debug draw ────────────────────────────────────────────────────────────────
void Path2D::_draw()
{
    Node2D::_draw();
    if (!show_debug || points.size() < 2) return;
    ensure_baked();

    // Draw the spline by walking consecutive LUT entries
    for (int i = 0; i + 1 < (int)m_lut.size(); ++i)
    {
        const Vec2 a = to_global(sample_local(m_lut[i].t));
        const Vec2 b = to_global(sample_local(m_lut[i + 1].t));
        DrawLineEx({a.x, a.y}, {b.x, b.y}, debug_width, debug_color);
    }
    // Close the loop
    if (loop && m_lut.size() >= 2)
    {
        const Vec2 last  = to_global(sample_local(m_lut.back().t));
        const Vec2 first = to_global(sample_local(m_lut.front().t));
        DrawLineEx({last.x, last.y}, {first.x, first.y}, debug_width,
                   Color{debug_color.r, debug_color.g, debug_color.b, 120});
    }
    // Control point markers
    for (const Vec2& p : points)
    {
        const Vec2 wp = to_global(p);
        DrawCircle((int)wp.x, (int)wp.y, 5, debug_color);
        DrawCircleLines((int)wp.x, (int)wp.y, 5, WHITE);
    }
}
