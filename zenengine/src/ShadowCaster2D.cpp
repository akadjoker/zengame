#include "ShadowCaster2D.hpp"

ShadowCaster2D::ShadowCaster2D(const std::string &p_name):
    Node2D(p_name)
{
    node_type = NodeType::ShadowCaster2D;
}

std::vector<Vec2> ShadowCaster2D::get_world_segments() const
{
    std::vector<Vec2> out;

    if (use_aabb)
    {
        // Build 4 edges of the AABB in local space, then transform to world.
        const float hw = size.x * 0.5f;
        const float hh = size.y * 0.5f;

        // Local corners
        const Vec2 corners[4] = {
            {-hw, -hh},
            { hw, -hh},
            { hw,  hh},
            {-hw,  hh}
        };

        for (int i = 0; i < 4; ++i)
        {
            out.push_back(local_to_world(corners[i]));
            out.push_back(local_to_world(corners[(i + 1) % 4]));
        }
    }
    else
    {
        // Custom segments — must be even count
        const size_t count = segments.size() & ~size_t(1);
        out.reserve(count);
        for (size_t i = 0; i < count; ++i)
            out.push_back(local_to_world(segments[i]));
    }

    return out;
}
