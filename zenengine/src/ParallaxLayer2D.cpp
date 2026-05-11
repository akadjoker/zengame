#include "pch.hpp"
#include "ParallaxLayer2D.hpp"
#include "Assets.hpp"
#include "SceneTree.hpp"
#include "View2D.hpp"

ParallaxLayer2D::ParallaxLayer2D(const std::string& p_name)
    : Node2D(p_name)
{
    node_type = NodeType::ParallaxLayer2D;
}

void ParallaxLayer2D::_ready()
{
    base_position = position;
}

void ParallaxLayer2D::_update(float /*dt*/)
{
    SceneTree* tree = get_tree();
    View2D* cam = tree ? tree->get_current_camera() : nullptr;
    if (!cam)
    {
        position = base_position;
        return;
    }

    const Vec2 cam_pos = cam->get_global_position();
    position = base_position;

    if (use_camera_x)
    {
        position.x += cam_pos.x * (1.0f - scroll_factor_x);
    }
    if (use_camera_y)
    {
        position.y += cam_pos.y * (1.0f - scroll_factor_y);
    }
}

void ParallaxLayer2D::_draw()
{
    if (graph < 0)
    {
        return;
    }

    Graph* g = GraphLib::Instance().getGraph(graph);
    if (!g)
    {
        return;
    }

    Texture2D* tex = GraphLib::Instance().getTexture(g->texture);
    if (!tex || tex->id <= 0)
    {
        return;
    }

    SceneTree* tree = get_tree();
    View2D* cam = tree ? tree->get_current_camera() : nullptr;
    Vec2 cam_pos = cam ? cam->get_global_position() : Vec2();

    float view_x = 0.0f;
    float view_y = 0.0f;
    float view_w = (float)GetScreenWidth();
    float view_h = (float)GetScreenHeight();
    if (cam)
    {
        cam->get_viewport_rect(view_x, view_y, view_w, view_h);
    }

    const float tex_w = (float)tex->width;
    const float tex_h = (float)tex->height;
    const float effective_scroll_x = cam_pos.x * scroll_factor_x;
    const float effective_scroll_y = cam_pos.y * scroll_factor_y;

    float uv_x1 = 0.0f;
    float uv_y1 = 0.0f;
    float uv_x2 = 1.0f;
    float uv_y2 = 1.0f;

    if ((mode & TILE_X) != 0)
    {
        uv_x1 = std::fmod(effective_scroll_x / tex_w, 1.0f);
        if (uv_x1 < 0.0f) uv_x1 += 1.0f;
        uv_x2 = uv_x1 + (view_w / tex_w);
    }

    if ((mode & TILE_Y) != 0)
    {
        uv_y1 = std::fmod(effective_scroll_y / tex_h, 1.0f);
        if (uv_y1 < 0.0f) uv_y1 += 1.0f;
        uv_y2 = uv_y1 + (view_h / tex_h);
    }

    if ((mode & STRETCH_X) != 0)
    {
        const float stretch_w = size.x > 0.0f ? size.x : tex_w;
        uv_x1 = effective_scroll_x / stretch_w;
        uv_x2 = (effective_scroll_x + view_w) / stretch_w;
    }

    if ((mode & STRETCH_Y) != 0)
    {
        const float stretch_h = size.y > 0.0f ? size.y : tex_h;
        uv_y1 = effective_scroll_y / stretch_h;
        uv_y2 = (effective_scroll_y + view_h) / stretch_h;
    }

    if ((mode & FLIP_X) != 0) std::swap(uv_x1, uv_x2);
    if ((mode & FLIP_Y) != 0) std::swap(uv_y1, uv_y2);

    SetTextureWrap(*tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(*tex, TEXTURE_FILTER_BILINEAR);

    const Rectangle src = {
        uv_x1 * tex_w,
        uv_y1 * tex_h,
        (uv_x2 - uv_x1) * tex_w,
        (uv_y2 - uv_y1) * tex_h
    };

    float dst_x = view_x;
    float dst_y = view_y;
    float dst_w = view_w;
    float dst_h = view_h;

    if ((mode & TILE_Y) == 0 && (mode & STRETCH_Y) == 0)
    {
        dst_h = size.y > 0.0f ? size.y : tex_h;
        dst_y = view_y + view_h - dst_h;
    }

    if ((mode & TILE_X) == 0 && (mode & STRETCH_X) == 0)
    {
        dst_w = size.x > 0.0f ? size.x : tex_w;
    }

    const Rectangle dst = {
        dst_x,
        dst_y,
        dst_w,
        dst_h
    };

    DrawTexturePro(*tex, src, dst, Vector2{0.0f, 0.0f}, 0.0f, color);
}
