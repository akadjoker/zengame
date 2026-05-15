#include "MotionStreak2D.hpp"
#include <rlgl.h>
#include <cmath>

MotionStreak2D::MotionStreak2D(const std::string& p_name) : Node2D(p_name)
{
    node_type = NodeType::MotionStreak2D;
}

void MotionStreak2D::reset()
{
    _points.clear();
    _initialized = false;
}

void MotionStreak2D::tint(Color new_color)
{
    color = new_color;
}

// ── _update ──────────────────────────────────────────────────────────────────

void MotionStreak2D::_update(float dt)
{
    Node2D::_update(dt);

    // Age all existing points
    for (auto& p : _points)
        p.age += dt;

    // Remove points that have fully faded
    while (!_points.empty() && _points.front().age >= time_to_fade)
        _points.erase(_points.begin());

    // Current world position of this node
    const Vec2 cur = get_global_position();

    if (!_initialized)
    {
        _last_pos    = cur;
        _initialized = true;
        _points.push_back({cur, 0.f});
        return;
    }

    // Add new point only if we moved enough
    const float dx = cur.x - _last_pos.x;
    const float dy = cur.y - _last_pos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);

    if (dist >= min_seg)
    {
        _points.push_back({cur, 0.f});
        _last_pos = cur;
    }
}

// ── helpers ───────────────────────────────────────────────────────────────────

// Returns the unit perpendicular (left-normal) of segment (points[i] → points[i+1])
static void seg_normal(const std::vector<MotionStreak2D::Point>& pts, int i,
                        float& out_nx, float& out_ny)
{
    float dx = pts[i + 1].pos.x - pts[i].pos.x;
    float dy = pts[i + 1].pos.y - pts[i].pos.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) { out_nx = 0.f; out_ny = -1.f; return; }
    out_nx = -dy / len;
    out_ny =  dx / len;
}

// ── _draw ─────────────────────────────────────────────────────────────────────

void MotionStreak2D::_draw()
{
    Node2D::_draw();

    const int n = (int)_points.size();
    if (n < 2) return;

    const float half = stroke_width * 0.5f;
    const float max_miter = half * 4.f;   // clamp spike limit

    // ── Pre-compute per-point miter offsets ─────────────────────────────────
    // At each interior point we average the normals of the two adjacent segments
    // and scale so the ribbon edge stays exactly half-width from the centreline.
    struct PtData { float ox, oy, alpha; };
    std::vector<PtData> pd(n);

    for (int i = 0; i < n; ++i)
    {
        // Alpha: newest point (age=0) → full, oldest → 0
        const float t = 1.f - _points[i].age / time_to_fade;
        pd[i].alpha = std::max(0.f, t) * (float)color.a;

        float nx, ny;
        if (i == 0)
        {
            seg_normal(_points, 0, nx, ny);
        }
        else if (i == n - 1)
        {
            seg_normal(_points, n - 2, nx, ny);
        }
        else
        {
            // Miter: bisector of the two adjacent segment normals
            float n1x, n1y, n2x, n2y;
            seg_normal(_points, i - 1, n1x, n1y);
            seg_normal(_points, i,     n2x, n2y);

            float mx = n1x + n2x;
            float my = n1y + n2y;
            float mlen = std::sqrt(mx * mx + my * my);

            if (mlen < 0.001f)
            {
                nx = n2x; ny = n2y;
            }
            else
            {
                mx /= mlen; my /= mlen;
                // Scale so the ribbon edge is exactly half from the centre:
                // miter_length = half / cos(angle) = half / dot(miter, n1)
                float dot = mx * n1x + my * n1y;
                float miter_len = (std::abs(dot) > 0.1f) ? half / dot : half;
                miter_len = std::min(miter_len, max_miter);
                pd[i].ox = mx * miter_len;
                pd[i].oy = my * miter_len;
                continue;
            }
        }
        pd[i].ox = nx * half;
        pd[i].oy = ny * half;
    }

    // ── Emit quads ────────────────────────────────────────────────────────────
    const bool has_tex = (texture.id > 0);
    rlSetTexture(has_tex ? texture.id : rlGetTextureIdDefault());
    rlBegin(RL_QUADS);

    for (int i = 0; i < n - 1; ++i)
    {
        const Vec2& pa = _points[i].pos;
        const Vec2& pb = _points[i + 1].pos;
        const PtData& a = pd[i];
        const PtData& b = pd[i + 1];

        const float u_a = (float)i       / (float)(n - 1);
        const float u_b = (float)(i + 1) / (float)(n - 1);

        const unsigned char aa = (unsigned char)(int)a.alpha;
        const unsigned char ab = (unsigned char)(int)b.alpha;

        // BL (bottom = -offset side)
        rlColor4ub(color.r, color.g, color.b, aa);
        rlTexCoord2f(u_a, 1.f);
        rlVertex3f(pa.x - a.ox, pa.y - a.oy, 0.f);
        // TL (top = +offset side)
        rlColor4ub(color.r, color.g, color.b, aa);
        rlTexCoord2f(u_a, 0.f);
        rlVertex3f(pa.x + a.ox, pa.y + a.oy, 0.f);
        // TR
        rlColor4ub(color.r, color.g, color.b, ab);
        rlTexCoord2f(u_b, 0.f);
        rlVertex3f(pb.x + b.ox, pb.y + b.oy, 0.f);
        // BR
        rlColor4ub(color.r, color.g, color.b, ab);
        rlTexCoord2f(u_b, 1.f);
        rlVertex3f(pb.x - b.ox, pb.y - b.oy, 0.f);
    }

    rlEnd();
    rlSetTexture(0);
}
