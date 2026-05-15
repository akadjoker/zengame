#pragma once

#include "Node2D.hpp"
#include <raylib.h>

// ----------------------------------------------------------------------------
// Rect2D — axis-aligned (or rotated, via Node2D transform) rectangle node.
//
// Usage:
//   auto* r = tree.create<Rect2D>(parent, "Wall");
//   r->size    = {200, 40};
//   r->color   = RED;
//   r->filled  = false;   // outline only
//   r->line_width = 2.0f;
// ----------------------------------------------------------------------------

class Rect2D : public Node2D
{
public:
    explicit Rect2D(const std::string& p_name = "Rect2D");

    Vec2  size        = {64.0f, 64.0f};  // local-space width/height
    Color color       = WHITE;
    bool  filled      = true;
    float line_width  = 1.0f;            // used when filled == false

    void _draw() override;
};
