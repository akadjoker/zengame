#include "Circle2D.hpp"

Circle2D::Circle2D(const std::string& p_name) : Node2D(p_name)
{
    node_type = NodeType::Circle2D;
}

void Circle2D::_draw()
{
    Node2D::_draw();

    // Centre on the node's local origin
    draw_circle({0.0f, 0.0f}, radius, color, filled);
}
