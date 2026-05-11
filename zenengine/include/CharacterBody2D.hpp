#pragma once

#include "CollisionObject2D.hpp"

struct CollisionInfo2D
{
    CollisionObject2D* collider = nullptr;
    Vec2 normal;
    float depth = 0.0f;
    bool hit = false;
};

class CharacterBody2D : public CollisionObject2D
{
public:

    explicit CharacterBody2D(const std::string& p_name = "CharacterBody2D");

    bool on_floor = false;
    bool on_wall = false;
    bool on_ceiling = false;
    Vec2 floor_normal;

    void move_topdown(const Vec2& velocity, float dt);
    bool place_free(float x, float y);
    CollisionObject2D* place_meeting(float x, float y);
    bool move_and_collide(const Vec2& motion, CollisionInfo2D* result = nullptr);
    bool move_and_collide(float vel_x, float vel_y, CollisionInfo2D* result = nullptr);
    bool move_and_stop(Vec2& velocity, float dt, CollisionInfo2D* result = nullptr);
    bool snap_to_floor(float snap_len, const Vec2& up_direction, Vec2& velocity);
    bool move_and_slide(Vec2& velocity, float dt, const Vec2& up_direction = Vec2(0.0f, -1.0f));
};
