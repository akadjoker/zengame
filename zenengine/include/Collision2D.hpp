#pragma once

#include "pch.hpp"
#include "math.hpp"
#include <raylib.h>
class Collider2D;

struct Contact2D
{
    bool  hit   = false;
    Vec2  normal;
    float depth = 0.0f;
    Vec2  point;        // approximate contact point in world space
};

namespace Collision2D
{
bool CollideAABB(const Rectangle& a, const Rectangle& b, Contact2D* out_contact = nullptr);
bool Collide(const Collider2D& a, const Collider2D& b, Contact2D* out_contact = nullptr);
bool CanCollide(uint32_t a_layer, uint32_t a_mask, uint32_t b_layer, uint32_t b_mask);
}
