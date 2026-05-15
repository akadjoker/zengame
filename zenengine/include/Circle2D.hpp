#pragma once

#include "Node2D.hpp"
#include <raylib.h>

// ----------------------------------------------------------------------------
// Circle2D — circle node centered on the node's local origin.
//
// Usage:
//   auto* c = tree.create<Circle2D>(parent, "Bullet");
//   c->radius  = 12.0f;
//   c->color   = YELLOW;
//   c->filled  = true;
// ----------------------------------------------------------------------------

class Circle2D : public Node2D
{
public:
    explicit Circle2D(const std::string& p_name = "Circle2D");

    float radius      = 32.0f;
    Color color       = WHITE;
    bool  filled      = true;
    float line_width  = 1.0f;  // used when filled == false

    void _draw() override;
};
