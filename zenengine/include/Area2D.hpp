#pragma once

#include "CollisionObject2D.hpp"

class Area2D : public CollisionObject2D
{
public:

    explicit Area2D(const std::string& p_name = "Area2D")
        : CollisionObject2D(p_name)
    {
        is_trigger = true;
    }
};

