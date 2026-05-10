#pragma once

#include "pch.hpp"
#include "CollisionObject2D.hpp"
#include "Assets.hpp"

class Collider2D;

class PolyMesh2D : public CollisionObject2D
{
public:

    explicit PolyMesh2D(const std::string& p_name = "PolyMesh2D");

    std::vector<Vec2> points;
    std::vector<unsigned short> indices;
    std::vector<Vec2> uvs;

    int body_graph = -1;
    int edge_graph = -1;
    float uv_scale = 1.0f;
    Color tint = WHITE;
    bool show_wire = false;
    Color wire_color = GREEN;
    bool debug_collision_mesh = false;
    Color debug_collision_color = RED;
    bool debug_collision_fill = false;
    bool auto_collision = true;
    bool auto_collision_visible = true;
    std::string auto_collider_name = "PolyMeshCollider";
    float body_scale_x = 1.0f;
    float body_scale_y = 1.0f;
    float edge_scale_x = 1.0f;
    float edge_scale_y = 1.0f;

    void clear();
    void add_point(float x, float y);
    bool build_polygon();
    bool build_track(float depth, float uv_scale_value = 1.0f);
    bool build_track_layered(float depth, float edge_width, float body_uv_scale, float edge_uv_scale,
                             float body_v_scale = 1.0f, float edge_v_scale = 1.0f);
    void set_texture_graph(int graph_id);
    void set_body_texture_graph(int graph_id);
    void set_edge_texture_graph(int graph_id);
    void set_top_scale(float sx, float sy);
    void set_bottom_scale(float sx, float sy);
    void rebuild_auto_collider();

    void _draw() override;

private:

    bool body_ready = false;
    bool edge_ready = false;
    std::vector<Vec2> body_top_strip;
    std::vector<Vec2> body_bottom_strip;
    std::vector<float> body_u_strip;
    float body_u_scale_strip = 1.0f;
    float body_v_scale_strip = 1.0f;
    std::vector<Vec2> edge_top_strip;
    std::vector<Vec2> edge_bottom_strip;
    std::vector<float> edge_u_strip;
    float edge_u_scale_strip = 1.0f;
    float edge_v_scale_strip = 1.0f;

    static Vec2 normalize_safe(const Vec2& v);
    static float polygon_signed_area(const std::vector<Vec2>& poly);
    static bool point_in_triangle_ccw(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c);
    static bool triangulate_ear_clipping(const std::vector<Vec2>& poly, std::vector<unsigned short>& out_indices);
    static Vec2 compute_track_normal(const std::vector<Vec2>& pts, int i);
    static std::vector<float> build_u_coords(const std::vector<Vec2>& pts, float uv_scale_value);
    static void draw_strip_immediate_2d(const std::vector<Vec2>& top,
                                        const std::vector<Vec2>& bottom,
                                        const std::vector<float>& u_coords,
                                        float u_scale,
                                        float v_scale,
                                        Texture2D* tex,
                                        const Matrix2D& xf,
                                        Color tint);

    void clear_auto_colliders();
    Collider2D* create_auto_collider(const std::vector<Vec2>& poly, int index);
};
