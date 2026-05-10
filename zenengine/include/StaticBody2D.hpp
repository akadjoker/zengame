#pragma once

#include "CollisionObject2D.hpp"

class StaticBody2D : public CollisionObject2D
{
public:

    explicit StaticBody2D(const std::string& p_name = "StaticBody2D")
        : CollisionObject2D(p_name)
    {
        is_trigger = false;
    }
};

