
#pragma once

#include "pch.hpp"
#include "Node2D.hpp"
#include "Assets.hpp"

// ============================================================
// Sprite2D.hpp  —  2D textured quad node
//
// Renders a single Graph (frame) from the GraphLib using the
// node's global transform (position, rotation, scale, pivot).
//
// Usage:
//   auto* spr = new Sprite2D("Player");
//   spr->graph_id = assets.load("player", "player.png");
//   spr->flip_x   = false;
//   spr->tint     = RColor::white();
//   player->add_child(spr);
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

    int     graph_id  = 0;      // id in GraphLib (0 = default checker)
    bool    flip_x    = false;
    bool    flip_y    = false;
    RColor  tint      = RColor::white();
    int     blend     = 0;      // blend mode (0 = normal)

    // Optional manual override for pivot.
    // If use_graph_pivot == true (default), pivot is taken from Graph::points[0].
    // Set use_graph_pivot = false and set pivot manually to override.
    bool    use_graph_pivot = true;

    // ----------------------------------------------------------
    // Node interface
    // ----------------------------------------------------------

    void _draw() override;

    // ----------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------

    // Returns the world-space AABB of this sprite (useful for culling)
    void get_bounds(float& out_x, float& out_y,
                    float& out_w, float& out_h) const;

protected:

    // Pointer to the shared GraphLib — set by SceneTree on _ready()
    // or assigned manually before the first _draw().
    GraphLib* m_lib = nullptr;

    friend class SceneTree;
};
