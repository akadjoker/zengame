#include "Bone2D.hpp"

Bone2D::Bone2D(const std::string& p_name) : Node2D(p_name)
{
    node_type = NodeType::Bone2D;
}

void Bone2D::save_rest_pose()
{
    rest_position = position;
    rest_rotation = rotation;
    rest_scale    = scale;
}

void Bone2D::reset_to_rest()
{
    position = rest_position;
    rotation = rest_rotation;
    scale    = rest_scale;
    invalidate_transform();
}

void Bone2D::_draw()
{
    Node2D::_draw();
}

void Bone2D::_draw_overlay()
{
    if (!show_bone || length <= 0.0f) return;

    // Classic bone shape: diamond near root, tapers to point at tip.
    // Spine convention: bone length runs along the LOCAL X axis.
    //
    //       root (0,0) ──── tip (length, 0)
    //
    // All in LOCAL space → engine applies global transform via to_global().

    const float hw  = thickness * 0.5f;         // half-width at base
    const float nub = thickness * 0.25f;         // small back nub

    const Vec2 tip   = {  length,             0.0f };
    const Vec2 left  = {  length * 0.15f,    -hw };
    const Vec2 right = {  length * 0.15f,     hw };
    const Vec2 back  = { -nub,                0.0f };

    // Front triangle (root → tip)
    draw_triangle(left, tip, right, bone_color, true);
    // Back nub triangle
    draw_triangle(left, back, right, {
        bone_color.r, bone_color.g, bone_color.b,
        (unsigned char)(bone_color.a / 2) }, true);

    // Outline
    const Color outline = {255, 255, 255, 80};
    draw_line(left,  tip,   outline, 1.0f);
    draw_line(right, tip,   outline, 1.0f);
    draw_line(left,  right, outline, 1.0f);

    // Root pivot circle
    draw_circle({0.0f, 0.0f}, hw * 0.6f, {255, 255, 255, 160}, true);
}

void Bone2D::propagate_draw()
{
    if (!visible) return;

    // Base: draws self (_draw) then children in order
    Node::propagate_draw();

    // Draw bone shape ON TOP of children (sprites)
    _draw_overlay();
}
