#include "pch.hpp"
#include "Collision2D.hpp"
#include "Collider2D.hpp"

namespace
{
    float Dot(const Vec2 &a, const Vec2 &b)
    {
        return a.x * b.x + a.y * b.y;
    }

    Vec2 Normalize(const Vec2 &v)
    {
        const float len = std::sqrt(v.x * v.x + v.y * v.y);
        if (len <= 1e-6f)
        {
            return Vec2(1.0f, 0.0f);
        }
        return Vec2(v.x / len, v.y / len);
    }

    void ProjectPolygon(const std::vector<Vec2> &pts, const Vec2 &axis, float &out_min, float &out_max)
    {
        if (pts.empty())
        {
            out_min = 0.0f;
            out_max = 0.0f;
            return;
        }
        out_min = Dot(pts[0], axis);
        out_max = out_min;
        for (size_t i = 1; i < pts.size(); ++i)
        {
            const float p = Dot(pts[i], axis);
            out_min = std::min(out_min, p);
            out_max = std::max(out_max, p);
        }
    }

    void ProjectCircle(const Vec2 &center, float radius, const Vec2 &axis, float &out_min, float &out_max)
    {
        const float c = Dot(center, axis);
        out_min = c - radius;
        out_max = c + radius;
    }

    Vec2 ComputeCenter(const std::vector<Vec2> &pts)
    {
        Vec2 c(0.0f, 0.0f);
        if (pts.empty())
        {
            return c;
        }
        for (const Vec2 &p : pts)
        {
            c += p;
        }
        c /= static_cast<float>(pts.size());
        return c;
    }

    bool TestAxesFromPoly(const std::vector<Vec2> &poly,
                          const std::vector<Vec2> &a,
                          const std::vector<Vec2> &b,
                          float &io_best_overlap, Vec2 &io_best_axis)
    {
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2 p0 = poly[i];
            const Vec2 p1 = poly[(i + 1) % poly.size()];
            const Vec2 edge = p1 - p0;
            const Vec2 axis = Normalize(Vec2(-edge.y, edge.x));

            float amin, amax, bmin, bmax;
            ProjectPolygon(a, axis, amin, amax);
            ProjectPolygon(b, axis, bmin, bmax);
            const float overlap = std::min(amax, bmax) - std::max(amin, bmin);
            if (overlap <= 0.0f)
            {
                return false;
            }
            if (overlap < io_best_overlap)
            {
                io_best_overlap = overlap;
                io_best_axis = axis;
            }
        }
        return true;
    }

    bool SATPolygonPolygon(const std::vector<Vec2> &a, const std::vector<Vec2> &b, Contact2D *out_contact)
    {
        float best_overlap = std::numeric_limits<float>::max();
        Vec2 best_axis(1.0f, 0.0f);

        if (!TestAxesFromPoly(a, a, b, best_overlap, best_axis))
            return false;
        if (!TestAxesFromPoly(b, a, b, best_overlap, best_axis))
            return false;

        Vec2 c1 = ComputeCenter(a);
        Vec2 c2 = ComputeCenter(b);
        if (Dot(c1 - c2, best_axis) < 0.0f)
        {
            best_axis = -best_axis;
        }
        if (out_contact)
        {
            out_contact->hit = true;
            out_contact->normal = best_axis;
            out_contact->depth = best_overlap;
        }
        return true;
    }

    bool SATCirclePolygon(const Vec2 &center, float radius, const std::vector<Vec2> &poly, Contact2D *out_contact)
    {
        float best_overlap = std::numeric_limits<float>::max();
        Vec2 best_axis(1.0f, 0.0f);

        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2 p0 = poly[i];
            const Vec2 p1 = poly[(i + 1) % poly.size()];
            const Vec2 edge = p1 - p0;
            const Vec2 axis = Normalize(Vec2(-edge.y, edge.x));

            float cmin, cmax, pmin, pmax;
            ProjectCircle(center, radius, axis, cmin, cmax);
            ProjectPolygon(poly, axis, pmin, pmax);
            const float overlap = std::min(cmax, pmax) - std::max(cmin, pmin);
            if (overlap <= 0.0f)
            {
                return false;
            }
            if (overlap < best_overlap)
            {
                best_overlap = overlap;
                best_axis = axis;
            }
        }

        float best_d2 = std::numeric_limits<float>::max();
        Vec2 best(1.0f, 0.0f);
        for (const Vec2 &p : poly)
        {
            const Vec2 d = center - p;
            const float d2 = d.x * d.x + d.y * d.y;
            if (d2 < best_d2)
            {
                best_d2 = d2;
                best = d;
            }
        }
        const Vec2 vertex_axis = Normalize(best);
        float vcmin, vcmax, vpmin, vpmax;
        ProjectCircle(center, radius, vertex_axis, vcmin, vcmax);
        ProjectPolygon(poly, vertex_axis, vpmin, vpmax);
        const float vertex_overlap = std::min(vcmax, vpmax) - std::max(vcmin, vpmin);
        if (vertex_overlap <= 0.0f)
        {
            return false;
        }
        if (vertex_overlap < best_overlap)
        {
            best_overlap = vertex_overlap;
            best_axis = vertex_axis;
        }

        Vec2 poly_center = ComputeCenter(poly);
        Vec2 dir = center - poly_center;
        if (Dot(dir, best_axis) < 0.0f)
        {
            best_axis = -best_axis;
        }

        if (out_contact)
        {
            out_contact->hit = true;
            out_contact->normal = best_axis;
            out_contact->depth = best_overlap;
        }
        return true;
    }

    // Normal do segmento (perpendicular, aponta para a esquerda do sentido a→b)
    Vec2 SegmentNormal(const Vec2 &a, const Vec2 &b)
    {
        return Normalize(Vec2(-(b.y - a.y), b.x - a.x));
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  Segmento × Círculo
    // ─────────────────────────────────────────────────────────────────────────────
    bool SATSegmentCircle(const Vec2 &sa, const Vec2 &sb,
                          const Vec2 &center, float radius,
                          bool one_sided,
                          Contact2D *out)
    {
        const Vec2 ab = sb - sa;
        const float len2 = ab.x * ab.x + ab.y * ab.y;
        const float t = len2 > 1e-10f
            ? std::max(0.0f, std::min(1.0f, Dot(center - sa, ab) / len2))
            : 0.0f;
        const bool is_endpoint = (t <= 0.0f || t >= 1.0f);
        const Vec2 closest = Vec2(sa.x + ab.x * t, sa.y + ab.y * t);
        const Vec2 delta = center - closest;
        const float dist2 = delta.x * delta.x + delta.y * delta.y;

        if (dist2 >= radius * radius)
            return false;

        const Vec2 seg_normal = SegmentNormal(sa, sb);

        // One-sided: só faz sentido na face do segmento, não nos endpoints.
        if (one_sided && !is_endpoint && Dot(delta, seg_normal) < 0.0f)
            return false;

        const float dist = std::sqrt(std::max(dist2, 1e-10f));
        const Vec2 n = dist > 1e-6f ? Vec2(delta.x / dist, delta.y / dist)
                                    : seg_normal;
        if (out)
        {
            out->hit = true;
            out->normal = n;
            out->depth = radius - dist;
        }
        return true;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  Segmento × Polígono  (SAT com 3 eixos)
    //
    //  Eixos testados:
    //    1. Normal do segmento          → separação perpendicular
    //    2. Tangente do segmento        → separação ao longo do comprimento (endpoints)
    //    3. Normais de todas as arestas do polígono
    // ─────────────────────────────────────────────────────────────────────────────
    bool SATSegmentPolygon(const Vec2 &sa, const Vec2 &sb,
                           const std::vector<Vec2> &poly,
                           bool one_sided,
                           Contact2D *out)
    {
        if (poly.size() < 3)
            return false;

        float best_overlap = std::numeric_limits<float>::max();
        Vec2 best_axis(1.0f, 0.0f);

        const std::vector<Vec2> seg_pts = {sa, sb};

        // Generic axis test for polygon edges (segment has non-zero extent on these)
        auto test_axis = [&](const Vec2 &axis) -> bool
        {
            float smin, smax, pmin, pmax;
            ProjectPolygon(seg_pts, axis, smin, smax);
            ProjectPolygon(poly, axis, pmin, pmax);
            const float overlap = std::min(smax, pmax) - std::max(smin, pmin);
            if (overlap <= 0.0f)
                return false;
            if (overlap < best_overlap)
            {
                best_overlap = overlap;
                best_axis = axis;
            }
            return true;
        };

        // — Segment normal axis (special case: segment is zero-thickness) ————————
        // Both endpoints project to the same value on the normal, so the generic
        // overlap formula always returns 0.  We compute penetration depth as the
        // minimum distance the polygon must move along the normal to exit the line.
        const Vec2 seg_normal = SegmentNormal(sa, sb);
        {
            const float seg_proj = Dot(sa, seg_normal); // == Dot(sb, seg_normal)
            float pmin, pmax;
            ProjectPolygon(poly, seg_normal, pmin, pmax);

            // Polygon entirely on one side → separating
            if (pmin >= seg_proj || pmax <= seg_proj)
                return false;

            // Polygon straddles the segment line — depth = min penetration side
            const float overlap = std::min(pmax - seg_proj, seg_proj - pmin);
            if (overlap < best_overlap)
            {
                best_overlap = overlap;
                best_axis = seg_normal;
            }
        }

        // — normais do polígono ——————————————————————————————————————————————————
        // Skip polygon edge normals that are (nearly) parallel to the segment normal
        // because the segment has zero-thickness on those axes.  The special case
        // above already handles separation in that direction.
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const Vec2 edge = poly[(i + 1) % poly.size()] - poly[i];
            const Vec2 axis = Normalize(Vec2(-edge.y, edge.x));
            const float dot_n = std::fabs(Dot(axis, seg_normal));
            if (dot_n > 0.98f)
                continue; // handled by segment normal special case
            if (!test_axis(axis))
                return false;
        }

        // — orientação da normal ——————————————————————————————————————————————————
        const Vec2 poly_center = ComputeCenter(poly);
        const Vec2 seg_center  = Vec2((sa.x + sb.x) * 0.5f, (sa.y + sb.y) * 0.5f);
        const Vec2 dir         = poly_center - seg_center;

        // One-sided: only collide if the polygon is in front of the segment
        // (i.e. on the side the normal points toward).
        if (one_sided && Dot(dir, seg_normal) < 0.0f)
        {
            // polygon is behind the segment — skip
            return false;
        }

        if (Dot(dir, best_axis) < 0.0f)
            best_axis = -best_axis;

        if (out)
        {
            out->hit = true;
            out->normal = best_axis;
            out->depth = best_overlap;
        }
        return true;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  Segmento × Segmento  (interseção + overlap perpendicular)
    // ─────────────────────────────────────────────────────────────────────────────
    bool SATSegmentSegment(const Vec2 &a0, const Vec2 &a1,
                           const Vec2 &b0, const Vec2 &b1,
                           Contact2D *out)
    {
        // Interseção geométrica exacta (cross product)
        const Vec2 da = a1 - a0;
        const Vec2 db = b1 - b0;
        const float dxb = da.x * db.y - da.y * db.x; // cross(da, db)

        if (std::fabs(dxb) > 1e-8f)
        {
            const Vec2 d = b0 - a0;
            const float t = (d.x * db.y - d.y * db.x) / dxb;
            const float u = (d.x * da.y - d.y * da.x) / dxb;

            if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f)
            {
                if (out)
                {
                    out->hit = true;
                    out->normal = Normalize(Vec2(-da.y, da.x));
                    out->depth = 0.0f; // interseção pura, sem penetração
                }
                return true;
            }
            return false;
        }

        // Paralelos / colineares — sem colisão para segmentos de espessura zero
        return false;
    }

}

namespace Collision2D
{
    bool CollideAABB(const Rectangle &a, const Rectangle &b, Contact2D *out_contact)
    {
        const float ax = a.x + a.width * 0.5f;
        const float ay = a.y + a.height * 0.5f;
        const float bx = b.x + b.width * 0.5f;
        const float by = b.y + b.height * 0.5f;

        const float dx = ax - bx;
        const float dy = ay - by;

        const float px = (a.width + b.width) * 0.5f - std::fabs(dx);
        if (px <= 0.0f)
        {
            return false;
        }

        const float py = (a.height + b.height) * 0.5f - std::fabs(dy);
        if (py <= 0.0f)
        {
            return false;
        }

        if (out_contact)
        {
            out_contact->hit = true;
            if (px < py)
            {
                out_contact->depth = px;
                out_contact->normal = Vec2(dx < 0.0f ? -1.0f : 1.0f, 0.0f);
            }
            else
            {
                out_contact->depth = py;
                out_contact->normal = Vec2(0.0f, dy < 0.0f ? -1.0f : 1.0f);
            }
        }

        return true;
    }

    bool CanCollide(uint32_t a_layer, uint32_t a_mask, uint32_t b_layer, uint32_t b_mask)
    {
        return (a_mask & b_layer) != 0 && (b_mask & a_layer) != 0;
    }

    bool Collide(const Collider2D &a, const Collider2D &b, Contact2D *out_contact)
    {
        Contact2D local_contact;
        if (!out_contact)
        {
            out_contact = &local_contact;
        }
        out_contact->hit = false;
        out_contact->depth = 0.0f;
        out_contact->normal = Vec2();

        if (!CheckCollisionRecs(a.get_world_aabb(), b.get_world_aabb()))
        {
            return false;
        }

        // ── Circle × Circle ───────────────────────────────────────────────────────
        if (a.shape == Collider2D::ShapeType::Circle && b.shape == Collider2D::ShapeType::Circle)
        {
            const Vec2 ca = a.get_world_center();
            const Vec2 cb = b.get_world_center();
            const float ra = a.get_world_radius();
            const float rb = b.get_world_radius();
            const Vec2 delta = ca - cb;
            const float dist2 = delta.x * delta.x + delta.y * delta.y;
            const float sum = ra + rb;
            if (dist2 >= sum * sum)
                return false;

            const float dist = std::sqrt(std::max(dist2, 1e-8f));
            out_contact->hit = true;
            out_contact->normal = dist > 1e-6f ? Vec2(delta.x / dist, delta.y / dist)
                                               : Vec2(1.0f, 0.0f);
            out_contact->depth = sum - dist;
            return true;
        }

        // ── Segment (antes de Circle×Polygon — Segment só tem 2 pontos) ───────────
        const bool a_is_seg = (a.shape == Collider2D::ShapeType::Segment);
        const bool b_is_seg = (b.shape == Collider2D::ShapeType::Segment);

        if (a_is_seg || b_is_seg)
        {
            // Segment × Segment
            if (a_is_seg && b_is_seg)
            {
                Vec2 a0, a1, b0, b1;
                a.get_world_segment(a0, a1);
                b.get_world_segment(b0, b1);
                return SATSegmentSegment(a0, a1, b0, b1, out_contact);
            }

            // Segment × Circle
            if (a_is_seg && b.shape == Collider2D::ShapeType::Circle)
            {
                Vec2 sa, sb;
                a.get_world_segment(sa, sb);
                return SATSegmentCircle(sa, sb,
                                        b.get_world_center(), b.get_world_radius(),
                                        a.one_sided, out_contact);
            }
            if (b_is_seg && a.shape == Collider2D::ShapeType::Circle)
            {
                Vec2 sa, sb;
                b.get_world_segment(sa, sb);
                bool hit = SATSegmentCircle(sa, sb,
                                            a.get_world_center(), a.get_world_radius(),
                                            b.one_sided, out_contact);
                if (hit && out_contact)
                    out_contact->normal = -out_contact->normal;
                return hit;
            }

            // Segment × Polygon
            if (a_is_seg)
            {
                Vec2 sa, sb;
                a.get_world_segment(sa, sb);
                std::vector<Vec2> bp;
                b.get_world_polygon(bp);
                return SATSegmentPolygon(sa, sb, bp, a.one_sided, out_contact);
            }
            else // b_is_seg
            {
                Vec2 sa, sb;
                b.get_world_segment(sa, sb);
                std::vector<Vec2> ap;
                a.get_world_polygon(ap);
                bool hit = SATSegmentPolygon(sa, sb, ap, b.one_sided, out_contact);
                if (hit && out_contact)
                    out_contact->normal = -out_contact->normal;
                return hit;
            }
        }

        // ── Circle × Polygon ──────────────────────────────────────────────────────
        if (a.shape == Collider2D::ShapeType::Circle)
        {
            std::vector<Vec2> poly;
            b.get_world_polygon(poly);
            if (poly.size() < 3)
                return false;
            return SATCirclePolygon(a.get_world_center(), a.get_world_radius(), poly, out_contact);
        }

        if (b.shape == Collider2D::ShapeType::Circle)
        {
            std::vector<Vec2> poly;
            a.get_world_polygon(poly);
            if (poly.size() < 3)
                return false;
            bool hit = SATCirclePolygon(b.get_world_center(), b.get_world_radius(), poly, out_contact);
            if (hit && out_contact)
                out_contact->normal = -out_contact->normal;
            return hit;
        }

        // ── Polygon × Polygon ─────────────────────────────────────────────────────
        std::vector<Vec2> ap;
        std::vector<Vec2> bp;
        a.get_world_polygon(ap);
        b.get_world_polygon(bp);
        if (ap.size() < 3 || bp.size() < 3)
            return false;
        return SATPolygonPolygon(ap, bp, out_contact);
    }
}
