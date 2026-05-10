
#include "pch.hpp"
#include "Sprite2D.hpp"
#include "render.hpp"
#include "Assets.hpp"

// ============================================================
// Sprite2D.cpp
// ============================================================

Sprite2D::Sprite2D(const std::string &p_name)
    : Node2D(p_name),
      graph(0),
      flip_x(false),
      flip_y(false),
      color(WHITE),
      blend(0),
      offset(0.0f, 0.0f),
      use_graph_pivot(true),
     clip_(false)
{
}

void Sprite2D::_draw()
{

    Graph *g = GraphLib::Instance().getGraph(graph);
    if (!g)
        return;
    Texture2D *rt = GraphLib::Instance().getTexture(g->texture);
    if (!rt)
        return;

    // Pivot: use graph control point [0] unless overridden
    if (use_graph_pivot && !g->points.empty())
        pivot = g->points[0];

    Matrix2D global = get_global_transform();

    if (!clip_)
    {

        clip_rect_.x = g->clip.x;
        clip_rect_.y = g->clip.y;
        clip_rect_.width = g->clip.width;
        clip_rect_.height = g->clip.height;
    }

    RenderTransformFlipClipOffset(
        *rt,
        offset,
        (float)g->width,
        (float)g->height,
        clip_rect_,
        flip_x,
        flip_y,
        color,
        &global,
        blend);
}

void Sprite2D::set_clip(const Rectangle &rect)
{
    clip_ = true;
    clip_rect_ = rect;
}

void Sprite2D::get_bounds(float &out_x, float &out_y,
                          float &out_w, float &out_h) const
{

    Graph *g = GraphLib::Instance().getGraph(graph);
    Vec2 gpos = get_global_position();
    out_x = gpos.x - pivot.x * scale.x;
    out_y = gpos.y - pivot.y * scale.y;
    out_w = (float)g->width * scale.x;
    out_h = (float)g->height * scale.y;
}
