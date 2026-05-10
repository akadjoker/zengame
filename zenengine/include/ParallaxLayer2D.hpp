#pragma once

#include "pch.hpp"
#include "Node2D.hpp"
#include <raylib.h>

class ParallaxLayer2D : public Node2D
{
public:

    enum Mode : uint32_t
    {
        TILE_X    = 1u << 0,
        TILE_Y    = 1u << 1,
        STRETCH_X = 1u << 2,
        STRETCH_Y = 1u << 3,
        FLIP_X    = 1u << 4,
        FLIP_Y    = 1u << 5
    };

    explicit ParallaxLayer2D(const std::string& p_name = "ParallaxLayer2D");

    int graph = -1;
    uint32_t mode = 0;
    float scroll_factor_x = 0.5f;
    float scroll_factor_y = 0.5f;
    Vec2 base_position = Vec2(0.0f, 0.0f);
    Vec2 size = Vec2(0.0f, 0.0f);
    Color color = WHITE;
    bool use_camera_x = true;
    bool use_camera_y = true;

    void _ready() override;
    void _update(float dt) override;
    void _draw() override;
};
