
#pragma once

#include "pch.hpp"
#include "Node2D.hpp"
#include <raylib.h>

// ============================================================
// Sprite2D.hpp  —  2D textured quad node
//
// Renders a single Graph (frame) from the GraphLib using the
// node's global transform (position, rotation, scale, pivot).
//
// ============================================================

class Sprite2D : public Node2D
{
public:

    // ----------------------------------------------------------
    // Constructor
    // ----------------------------------------------------------

    explicit Sprite2D(const std::string& p_name = "Sprite2D");

    // ----------------------------------------------------------
    // Properties
    // ----------------------------------------------------------

    int     graph  = 0;      // id in GraphLib (0 = default checker)
    bool    flip_x    = false;
    bool    flip_y    = false;
    Color   color      =  WHITE;   
    int     blend     = 0;      // blend mode (0 = normal)
    Vec2    offset;

    // Optional manual override for pivot.
    // If use_graph_pivot == true (default), pivot is taken from Graph::points[0].
    // Set use_graph_pivot = false and set pivot manually to override.
    bool    use_graph_pivot = true;

    // ----------------------------------------------------------
    // Node interface
    // ----------------------------------------------------------

    void _draw() override;


    void set_clip(const Rectangle& rect);

    // ----------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------

    // Returns the world-space AABB of this sprite (useful for culling)
    void get_bounds(float& out_x, float& out_y,
                    float& out_w, float& out_h) const;

protected:

    bool  clip_;
    Rectangle clip_rect_;


    friend class SceneTree;
};
