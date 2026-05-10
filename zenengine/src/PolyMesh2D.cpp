#include "pch.hpp"
#include "PolyMesh2D.hpp"
#include "Collider2D.hpp"
#include <poly2tri.h>
#include <rlgl.h>

namespace
{
bool same_point(const Vec2& a, const Vec2& b, float eps = 1e-4f)
{
    return (std::fabs(a.x - b.x) <= eps) && (std::fabs(a.y - b.y) <= eps);
}

float cross2d(const Vec2& a, const Vec2& b, const Vec2& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool clean_ring_points(const std::vector<Vec2>& in, std::vector<Vec2>& out)
{
    out.clear();
    if (in.empty()) return false;

    out.reserve(in.size());
    for (const Vec2& p : in)
    {
        if (out.empty() || !same_point(out.back(), p))
        {
            out.push_back(p);
        }
    }

    if (out.size() >= 2 && same_point(out.front(), out.back()))
    {
        out.pop_back();
    }
    if (out.size() < 3) return false;

    bool changed = true;
    int pass = 0;
    while (changed && out.size() >= 3 && pass < 64)
    {
        changed = false;
        std::vector<Vec2> next;
        const int n = (int)out.size();
        next.reserve((size_t)n);

        for (int i = 0; i < n; ++i)
        {
            const Vec2& prev = out[(size_t)((i + n - 1) % n)];
            const Vec2& curr = out[(size_t)i];
            const Vec2& nxt = out[(size_t)((i + 1) % n)];

            if (same_point(prev, curr) || same_point(curr, nxt) || std::fabs(cross2d(prev, curr, nxt)) <= 1e-5f)
            {
                changed = true;
                continue;
            }
            next.push_back(curr);
        }

        if (next.size() < 3)
        {
            out.clear();
            return false;
        }

        if (next.size() != out.size())
        {
            out = next;
        }
        ++pass;
    }

    return out.size() >= 3;
}

int find_or_add_vertex(std::vector<Vec2>& vertices, const Vec2& p, float eps = 1e-4f)
{
    for (int i = 0; i < (int)vertices.size(); ++i)
    {
        if (std::fabs(vertices[(size_t)i].x - p.x) <= eps && std::fabs(vertices[(size_t)i].y - p.y) <= eps)
        {
            return i;
        }
    }

    if (vertices.size() >= 65535) return -1;
    vertices.push_back(p);
    return (int)vertices.size() - 1;
}

bool triangulate_polygon_poly2tri(const std::vector<Vec2>& poly,
                                  std::vector<Vec2>& out_vertices,
                                  std::vector<unsigned short>& out_indices)
{
    out_vertices.clear();
    out_indices.clear();
    if (poly.size() < 3 || poly.size() > 65535) return false;

    out_vertices = poly;

    std::vector<p2t::Point*> p2points;
    p2points.reserve(poly.size());
    for (const Vec2& p : poly)
    {
        p2points.push_back(new p2t::Point((double)p.x, (double)p.y));
    }

    bool ok = false;
    try
    {
        p2t::CDT cdt(p2points);
        cdt.Triangulate();
        std::vector<p2t::Triangle*> tris = cdt.GetTriangles();
        out_indices.reserve(tris.size() * 3);

        for (p2t::Triangle* tri : tris)
        {
            if (!tri) continue;
            p2t::Point* a = tri->GetPoint(0);
            p2t::Point* b = tri->GetPoint(1);
            p2t::Point* c = tri->GetPoint(2);
            if (!a || !b || !c) continue;

            const int ia = find_or_add_vertex(out_vertices, Vec2((float)a->x, (float)a->y));
            const int ib = find_or_add_vertex(out_vertices, Vec2((float)b->x, (float)b->y));
            const int ic = find_or_add_vertex(out_vertices, Vec2((float)c->x, (float)c->y));
            if (ia < 0 || ib < 0 || ic < 0) continue;

            out_indices.push_back((unsigned short)ia);
            out_indices.push_back((unsigned short)ib);
            out_indices.push_back((unsigned short)ic);
        }
        ok = !out_indices.empty();
    }
    catch (...)
    {
        ok = false;
    }

    for (p2t::Point* pt : p2points) delete pt;
    return ok;
}
}

PolyMesh2D::PolyMesh2D(const std::string& p_name)
    : CollisionObject2D(p_name)
{
}

Vec2 PolyMesh2D::normalize_safe(const Vec2& v)
{
    const float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len <= 0.0001f) return Vec2(0.0f, -1.0f);
    return Vec2(v.x / len, v.y / len);
}

float PolyMesh2D::polygon_signed_area(const std::vector<Vec2>& poly)
{
    if (poly.size() < 3) return 0.0f;
    double area = 0.0;
    const int n = (int)poly.size();
    for (int i = 0; i < n; ++i)
    {
        const Vec2& a = poly[(size_t)i];
        const Vec2& b = poly[(size_t)((i + 1) % n)];
        area += (double)a.x * (double)b.y - (double)b.x * (double)a.y;
    }
    return (float)(area * 0.5);
}

bool PolyMesh2D::point_in_triangle_ccw(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c)
{
    const float eps = 1e-6f;
    const float c1 = cross2d(a, b, p);
    const float c2 = cross2d(b, c, p);
    const float c3 = cross2d(c, a, p);
    return (c1 >= -eps) && (c2 >= -eps) && (c3 >= -eps);
}

bool PolyMesh2D::triangulate_ear_clipping(const std::vector<Vec2>& poly, std::vector<unsigned short>& out_indices)
{
    out_indices.clear();
    if (poly.size() < 3 || poly.size() > 65535) return false;

    const int n = (int)poly.size();
    std::vector<int> V((size_t)n);

    if (polygon_signed_area(poly) > 0.0f)
    {
        for (int i = 0; i < n; ++i) V[(size_t)i] = i;
    }
    else
    {
        for (int i = 0; i < n; ++i) V[(size_t)i] = (n - 1 - i);
    }

    int nv = n;
    int count = 2 * nv;
    int v = nv - 1;

    while (nv > 2)
    {
        if ((count--) <= 0)
        {
            out_indices.clear();
            return false;
        }

        int u = v;
        if (u >= nv) u = 0;
        v = u + 1;
        if (v >= nv) v = 0;
        int w = v + 1;
        if (w >= nv) w = 0;

        const int ia = V[(size_t)u];
        const int ib = V[(size_t)v];
        const int ic = V[(size_t)w];

        const Vec2& a = poly[(size_t)ia];
        const Vec2& b = poly[(size_t)ib];
        const Vec2& c = poly[(size_t)ic];
        if (cross2d(a, b, c) <= 1e-6f) continue;

        bool ear = true;
        for (int p = 0; p < nv; ++p)
        {
            if (p == u || p == v || p == w) continue;
            if (point_in_triangle_ccw(poly[(size_t)V[(size_t)p]], a, b, c))
            {
                ear = false;
                break;
            }
        }
        if (!ear) continue;

        out_indices.push_back((unsigned short)ia);
        out_indices.push_back((unsigned short)ib);
        out_indices.push_back((unsigned short)ic);

        for (int s = v, t = v + 1; t < nv; ++s, ++t)
        {
            V[(size_t)s] = V[(size_t)t];
        }
        nv--;
        count = 2 * nv;
    }

    return !out_indices.empty();
}

Vec2 PolyMesh2D::compute_track_normal(const std::vector<Vec2>& pts, int i)
{
    const int n = (int)pts.size();
    Vec2 dir(1.0f, 0.0f);

    if (n >= 2)
    {
        if (i <= 0) dir = pts[1] - pts[0];
        else if (i >= n - 1) dir = pts[(size_t)(n - 1)] - pts[(size_t)(n - 2)];
        else dir = pts[(size_t)(i + 1)] - pts[(size_t)(i - 1)];
    }

    dir = normalize_safe(dir);
    Vec2 normal(dir.y, -dir.x);
    if (normal.y > 0.0f)
    {
        normal = -normal;
    }
    return normalize_safe(normal);
}

std::vector<float> PolyMesh2D::build_u_coords(const std::vector<Vec2>& pts, float uv_scale_value)
{
    std::vector<float> u(pts.size(), 0.0f);
    if (pts.empty()) return u;

    float acc = 0.0f;
    for (size_t i = 1; i < pts.size(); ++i)
    {
        const Vec2 d = pts[i] - pts[i - 1];
        acc += std::sqrt(d.x * d.x + d.y * d.y);
        u[i] = acc * uv_scale_value;
    }
    return u;
}

void PolyMesh2D::clear_auto_colliders()
{
    std::vector<Node*> to_delete;
    for (size_t i = 0; i < get_child_count(); ++i)
    {
        Node* n = get_child(i);
        Collider2D* c = dynamic_cast<Collider2D*>(n);
        if (!c) continue;
        if (c->name.rfind(auto_collider_name, 0) == 0)
        {
            to_delete.push_back(n);
        }
    }

    for (Node* n : to_delete)
    {
        destroy_child(n);
    }
}

Collider2D* PolyMesh2D::create_auto_collider(const std::vector<Vec2>& poly, int index)
{
    if (poly.size() < 3) return nullptr;

    Collider2D* c = new Collider2D(auto_collider_name + "_" + std::to_string(index));
    c->shape = Collider2D::ShapeType::Polygon;
    c->points = poly;
    c->visible = auto_collision_visible;
    add_child(c);
    return c;
}

void PolyMesh2D::clear()
{
    clear_auto_colliders();

    points.clear();
    indices.clear();
    uvs.clear();

    body_top_strip.clear();
    body_bottom_strip.clear();
    body_u_strip.clear();
    edge_top_strip.clear();
    edge_bottom_strip.clear();
    edge_u_strip.clear();

    body_u_scale_strip = 1.0f;
    body_v_scale_strip = 1.0f;
    edge_u_scale_strip = 1.0f;
    edge_v_scale_strip = 1.0f;

    body_ready = false;
    edge_ready = false;
}

void PolyMesh2D::add_point(float x, float y)
{
    points.push_back(Vec2(x, y));
}

bool PolyMesh2D::build_track(float depth, float uv_scale_value)
{
    return build_track_layered(depth, 0.0f, uv_scale_value, uv_scale_value, 1.0f, 1.0f);
}

bool PolyMesh2D::build_track_layered(float depth, float edge_width, float body_uv_scale, float edge_uv_scale,
                                     float body_v_scale, float edge_v_scale)
{
    if (points.size() < 2) return false;

    indices.clear();
    uvs.clear();
    body_top_strip.clear();
    body_bottom_strip.clear();
    body_u_strip.clear();
    edge_top_strip.clear();
    edge_bottom_strip.clear();
    edge_u_strip.clear();
    body_u_scale_strip = 1.0f;
    body_v_scale_strip = 1.0f;
    edge_u_scale_strip = 1.0f;
    edge_v_scale_strip = 1.0f;
    body_ready = false;
    edge_ready = false;

    if (body_v_scale <= 0.0f) body_v_scale = 1.0f;
    if (edge_v_scale <= 0.0f) edge_v_scale = 1.0f;

    if (depth > 0.0f)
    {
        std::vector<Vec2> body_top = points;
        std::vector<Vec2> body_bottom = points;
        float max_top_y = body_top[0].y;
        for (size_t i = 1; i < body_top.size(); ++i)
        {
            max_top_y = std::max(max_top_y, body_top[i].y);
        }
        const float flat_bottom_y = max_top_y + depth;
        for (Vec2& p : body_bottom)
        {
            p.y = flat_bottom_y;
        }

        body_top_strip = body_top;
        body_bottom_strip = body_bottom;
        body_u_strip = build_u_coords(points, 1.0f);
        body_u_scale_strip = body_uv_scale;
        body_v_scale_strip = body_v_scale;
        body_ready = body_top_strip.size() >= 2;
    }

    if (edge_width > 0.0f)
    {
        const float half = edge_width * 0.5f;
        edge_top_strip.resize(points.size());
        edge_bottom_strip.resize(points.size());

        for (int i = 0; i < (int)points.size(); ++i)
        {
            const Vec2 n = compute_track_normal(points, i);
            edge_top_strip[(size_t)i] = points[(size_t)i] + n * half;
            edge_bottom_strip[(size_t)i] = points[(size_t)i] - n * half;
        }

        edge_u_strip = build_u_coords(points, 1.0f);
        edge_u_scale_strip = edge_uv_scale;
        edge_v_scale_strip = edge_v_scale;
        edge_ready = edge_top_strip.size() >= 2;
    }

    rebuild_auto_collider();
    return body_ready || edge_ready;
}

bool PolyMesh2D::build_polygon()
{
    if (points.size() < 3) return false;

    body_top_strip.clear();
    body_bottom_strip.clear();
    body_u_strip.clear();
    edge_top_strip.clear();
    edge_bottom_strip.clear();
    edge_u_strip.clear();
    body_ready = false;
    edge_ready = false;
    indices.clear();
    uvs.clear();

    std::vector<Vec2> clean;
    if (!clean_ring_points(points, clean))
    {
        return false;
    }

    std::vector<Vec2> mesh_vertices;
    std::vector<unsigned short> tri_indices;
    if (!triangulate_polygon_poly2tri(clean, mesh_vertices, tri_indices))
    {
        if (!triangulate_ear_clipping(clean, tri_indices))
        {
            return false;
        }
        mesh_vertices = clean;
    }

    points = mesh_vertices;
    indices = tri_indices;
    const float base_x = points[0].x;
    const float base_y = points[0].y;
    uvs.reserve(points.size());
    for (const Vec2& p : points)
    {
        uvs.push_back(Vec2((p.x - base_x) * uv_scale, (p.y - base_y) * uv_scale));
    }

    body_ready = true;
    rebuild_auto_collider();
    return true;
}

void PolyMesh2D::set_texture_graph(int graph_id)
{
    body_graph = graph_id;
    edge_graph = graph_id;
}

void PolyMesh2D::set_body_texture_graph(int graph_id)
{
    body_graph = graph_id;
}

void PolyMesh2D::set_edge_texture_graph(int graph_id)
{
    edge_graph = graph_id;
}

void PolyMesh2D::set_top_scale(float sx, float sy)
{
    if (sx <= 0.0f) sx = 1.0f;
    if (sy <= 0.0f) sy = 1.0f;
    edge_scale_x = sx;
    edge_scale_y = sy;
}

void PolyMesh2D::set_bottom_scale(float sx, float sy)
{
    if (sx <= 0.0f) sx = 1.0f;
    if (sy <= 0.0f) sy = 1.0f;
    body_scale_x = sx;
    body_scale_y = sy;
}

void PolyMesh2D::rebuild_auto_collider()
{
    clear_auto_colliders();
    if (!auto_collision) return;

    const bool has_track_strip = body_ready && body_top_strip.size() >= 2 
                               && body_bottom_strip.size() == body_top_strip.size();
    if (has_track_strip)
    {
        // Usa edge_top_strip se disponível, senão cai no body_top_strip
        const std::vector<Vec2>& col_top = (edge_ready && edge_top_strip.size() == body_top_strip.size())
                                            ? edge_top_strip
                                            : body_top_strip;

        int collider_index = 0;
        for (size_t i = 0; i + 1 < col_top.size(); ++i)
        {
            std::vector<Vec2> quad = {
                col_top[i],
                body_bottom_strip[i],
                body_bottom_strip[i + 1],
                col_top[i + 1]
            };
            if (polygon_signed_area(quad) < 0.0f) std::reverse(quad.begin(), quad.end());
            create_auto_collider(quad, collider_index++);
        }
        return;
    }
    if (points.size() < 3 || indices.size() < 3) return;

    int collider_index = 0;
    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        const unsigned short ia = indices[i];
        const unsigned short ib = indices[i + 1];
        const unsigned short ic = indices[i + 2];
        if (ia >= points.size() || ib >= points.size() || ic >= points.size()) continue;

        std::vector<Vec2> tri = {points[(size_t)ia], points[(size_t)ib], points[(size_t)ic]};
        if (polygon_signed_area(tri) < 0.0f) std::swap(tri[1], tri[2]);
        create_auto_collider(tri, collider_index++);
    }
}

void PolyMesh2D::draw_strip_immediate_2d(const std::vector<Vec2>& top,
                                         const std::vector<Vec2>& bottom,
                                         const std::vector<float>& u_coords,
                                         float u_scale,
                                         float v_scale,
                                         Texture2D* tex,
                                         const Matrix2D& xf,
                                         Color tint)
{
    if (top.size() < 2 || bottom.size() != top.size() || u_coords.size() != top.size()) return;
    if (!tex || tex->id <= 0) return;
    if (u_scale <= 0.0f) u_scale = 1.0f;
    if (v_scale <= 0.0f) v_scale = 1.0f;

    const int n = (int)top.size();
    rlCheckRenderBatchLimit((n - 1) * 4);
    rlSetTexture(tex->id);
    rlBegin(RL_QUADS);
    rlNormal3f(0.0f, 0.0f, 1.0f);

    for (int i = 0; i < n - 1; ++i)
    {
        const float u1 = u_coords[(size_t)i] * u_scale;
        const float u2 = u_coords[(size_t)i + 1] * u_scale;

        const Vec2 v1 = xf.TransformCoords(top[(size_t)i]);
        const Vec2 v0 = xf.TransformCoords(bottom[(size_t)i]);
        const Vec2 v3 = xf.TransformCoords(bottom[(size_t)i + 1]);
        const Vec2 v2 = xf.TransformCoords(top[(size_t)i + 1]);

        rlColor4ub(tint.r, tint.g, tint.b, tint.a);
        rlTexCoord2f(u1, 0.0f);    rlVertex3f(v1.x, v1.y, 0.0f);
        rlTexCoord2f(u1, v_scale); rlVertex3f(v0.x, v0.y, 0.0f);
        rlTexCoord2f(u2, v_scale); rlVertex3f(v3.x, v3.y, 0.0f);
        rlTexCoord2f(u2, 0.0f);    rlVertex3f(v2.x, v2.y, 0.0f);
    }

    rlEnd();
    rlSetTexture(0);
}

void PolyMesh2D::_draw()
{
    auto draw_collision_debug = [&]()
    {
        if (!debug_collision_mesh) return;

        std::vector<Collider2D*> cols;
        get_colliders(cols);
        for (Collider2D* col : cols)
        {
            if (!col) continue;
            if (col->name.rfind(auto_collider_name, 0) != 0) continue;

            std::vector<Vec2> poly = col->points;
            if (poly.size() < 3) continue;

            const Matrix2D xf_dbg = col->get_global_transform();
            if (debug_collision_fill)
            {
                for (size_t i = 1; i + 1 < poly.size(); ++i)
                {
                    const Vec2 a = xf_dbg.TransformCoords(poly[0]);
                    const Vec2 b = xf_dbg.TransformCoords(poly[i]);
                    const Vec2 c = xf_dbg.TransformCoords(poly[i + 1]);
                    DrawTriangle(Vector2{a.x, a.y}, Vector2{b.x, b.y}, Vector2{c.x, c.y}, ColorAlpha(debug_collision_color, 0.20f));
                }
            }

            for (size_t i = 0; i < poly.size(); ++i)
            {
                const Vec2 a = xf_dbg.TransformCoords(poly[i]);
                const Vec2 b = xf_dbg.TransformCoords(poly[(i + 1) % poly.size()]);
                DrawLineV(Vector2{a.x, a.y}, Vector2{b.x, b.y}, debug_collision_color);
            }
        }
    };

    const bool has_body_strip = body_ready && body_top_strip.size() >= 2 && body_bottom_strip.size() == body_top_strip.size();
    const bool has_edge_strip = edge_ready && edge_top_strip.size() >= 2 && edge_bottom_strip.size() == edge_top_strip.size();

    if (has_body_strip || has_edge_strip)
    {
        Texture2D* btex = nullptr;
        Texture2D* etex = nullptr;
        if (body_graph >= 0)
        {
            if (Graph* g = GraphLib::Instance().getGraph(body_graph)) btex = GraphLib::Instance().getTexture(g->texture);
        }
        if (edge_graph >= 0)
        {
            if (Graph* g = GraphLib::Instance().getGraph(edge_graph)) etex = GraphLib::Instance().getTexture(g->texture);
        }

        if (btex && btex->id > 0)
        {
            SetTextureWrap(*btex, TEXTURE_WRAP_REPEAT);
            SetTextureFilter(*btex, TEXTURE_FILTER_POINT);
        }
        if (etex && etex->id > 0)
        {
            SetTextureWrap(*etex, TEXTURE_WRAP_REPEAT);
            SetTextureFilter(*etex, TEXTURE_FILTER_BILINEAR);
        }

        const Matrix2D xf = get_global_transform();
        if (has_body_strip)
        {
            draw_strip_immediate_2d(body_top_strip, body_bottom_strip, body_u_strip,
                                    body_u_scale_strip * body_scale_x,
                                    body_v_scale_strip * body_scale_y,
                                    btex, xf, tint);
        }
        if (has_edge_strip)
        {
            draw_strip_immediate_2d(edge_top_strip, edge_bottom_strip, edge_u_strip,
                                    edge_u_scale_strip * edge_scale_x,
                                    edge_v_scale_strip * edge_scale_y,
                                    etex ? etex : btex, xf, tint);
        }

        if (show_wire)
        {
            auto draw_polyline = [&](const std::vector<Vec2>& pts)
            {
                for (size_t i = 1; i < pts.size(); ++i)
                {
                    const Vec2 a = xf.TransformCoords(pts[i - 1]);
                    const Vec2 b = xf.TransformCoords(pts[i]);
                    DrawLineV(Vector2{a.x, a.y}, Vector2{b.x, b.y}, wire_color);
                }
            };
            if (!body_top_strip.empty()) draw_polyline(body_top_strip);
            if (!body_bottom_strip.empty()) draw_polyline(body_bottom_strip);
            if (!edge_top_strip.empty()) draw_polyline(edge_top_strip);
            if (!edge_bottom_strip.empty()) draw_polyline(edge_bottom_strip);
        }

        draw_collision_debug();
        return;
    }

    if (points.size() < 3 || indices.size() < 3)
    {
        draw_collision_debug();
        return;
    }

    std::vector<Vec2> wpts;
    wpts.reserve(points.size());
    const Matrix2D g = get_global_transform();
    for (const Vec2& p : points) wpts.push_back(g.TransformCoords(p));

    Texture2D* tex = nullptr;
    if (body_graph >= 0)
    {
        if (Graph* graph = GraphLib::Instance().getGraph(body_graph)) tex = GraphLib::Instance().getTexture(graph->texture);
    }
    if (tex && tex->id > 0)
    {
        SetTextureWrap(*tex, TEXTURE_WRAP_REPEAT);
        SetTextureFilter(*tex, TEXTURE_FILTER_POINT);
    }

    if (tex && tex->id > 0 && uvs.size() == points.size())
    {
        rlCheckRenderBatchLimit((int)indices.size());
        rlSetTexture(tex->id);
        rlBegin(RL_TRIANGLES);
        rlNormal3f(0.0f, 0.0f, 1.0f);
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            for (int k = 0; k < 3; ++k)
            {
                const unsigned short idx = indices[i + (size_t)k];
                const Vec2& p = wpts[(size_t)idx];
                const Vec2& uv = uvs[(size_t)idx];
                rlColor4ub(tint.r, tint.g, tint.b, tint.a);
                rlTexCoord2f(uv.x, uv.y);
                rlVertex3f(p.x, p.y, 0.0f);
            }
        }
        rlEnd();
        rlSetTexture(0);
    }
    else
    {
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const Vec2& a = wpts[(size_t)indices[i]];
            const Vec2& b = wpts[(size_t)indices[i + 1]];
            const Vec2& c = wpts[(size_t)indices[i + 2]];
            DrawTriangle(Vector2{a.x, a.y}, Vector2{b.x, b.y}, Vector2{c.x, c.y}, tint);
        }
    }

    if (show_wire)
    {
        for (size_t i = 0; i < wpts.size(); ++i)
        {
            const Vec2& a = wpts[i];
            const Vec2& b = wpts[(i + 1) % wpts.size()];
            DrawLineV(Vector2{a.x, a.y}, Vector2{b.x, b.y}, wire_color);
        }
    }

    draw_collision_debug();
}
