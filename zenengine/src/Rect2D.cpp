#include "Rect2D.hpp"

Rect2D::Rect2D(const std::string& p_name) : Node2D(p_name)
{
    node_type = NodeType::Rect2D;
}

void Rect2D::_draw()
{
    Node2D::_draw();

    // Use the Node2D helper — handles rotation/scale via to_global()
    // origin at (-size/2, -size/2) so the rect is centred on the node position
    const Vec2 half  = {size.x * 0.5f, size.y * 0.5f};
    const Vec2 origin = {-half.x, -half.y};

    if (filled)
    {
        draw_rect(origin, size, color, true);
    }
    else
    {
        // Outline: draw four sides with line_width
        const Vec2 tl = origin;
        const Vec2 tr = {origin.x + size.x, origin.y};
        const Vec2 br = {origin.x + size.x, origin.y + size.y};
        const Vec2 bl = {origin.x,           origin.y + size.y};
        draw_line(tl, tr, color, line_width);
        draw_line(tr, br, color, line_width);
        draw_line(br, bl, color, line_width);
        draw_line(bl, tl, color, line_width);
    }
}
