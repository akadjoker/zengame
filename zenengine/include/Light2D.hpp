#pragma once

#include "pch.hpp"
#include "Node2D.hpp"

class Light2D : public Node2D
{
public:
    enum class Type
    {
        Point,
        Spot
    };

    explicit Light2D(const std::string& p_name = "Light2D");

    Type  type = Type::Point;
    bool  enabled = true;
    bool cast_shadows = true;
    Color color = WHITE;
    float intensity = 1.0f;
    float radius = 256.0f;
    float spot_angle = 45.0f;
    float spot_dir = 0.0f;
};
