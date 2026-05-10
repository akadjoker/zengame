#pragma once

#include "pch.hpp"
#include "Node2D.hpp"
#include <vector>

// Attach to any node that should cast shadows.
// By default builds an AABB from `size`. For custom shapes, populate `segments`
// directly — each pair of Vec2 is a world-space edge (start, end).
class ShadowCaster2D : public Node2D
{
public:

        explicit ShadowCaster2D(const std::string& p_name = "ShadowCaster2D");
    
 

    // If true, the AABB defined by `size` is used and `segments` is ignored.
    bool  use_aabb = true;
    Vec2  size     = {32.0f, 32.0f};   // local-space AABB half-extents * 2

    // Custom polygon edges in LOCAL space (pairs: [a0,b0, a1,b1, ...]).
    // Only used when use_aabb = false.
    std::vector<Vec2> segments;

    bool enabled = true;

    // Returns world-space segment list (computed each frame from transform).
    // Each consecutive pair is one edge: [p0, p1, p2, p3, ...] → edges (p0,p1), (p2,p3)
    std::vector<Vec2> get_world_segments() const;
};
