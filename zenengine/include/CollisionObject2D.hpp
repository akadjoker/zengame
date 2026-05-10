#pragma once

#include "pch.hpp"
#include "Node2D.hpp"

class Collider2D;

class CollisionObject2D : public Node2D
{
public:

    explicit CollisionObject2D(const std::string& p_name = "CollisionObject2D");

    bool enabled = true;
    bool is_trigger = false;
    uint32_t collision_layer = 1u;
    uint32_t collision_mask = 0xFFFFFFFFu;

    void set_collision_layer_bit(int bit);
    void set_collision_mask_bit(int bit);
    void add_collision_mask_bit(int bit);
    void remove_collision_mask_bit(int bit);

    Collider2D* get_collider() const;
    void get_colliders(std::vector<Collider2D*>& out) const;

    virtual void on_collision_enter(CollisionObject2D* other);
    virtual void on_collision_stay(CollisionObject2D* other);
    virtual void on_collision_exit(CollisionObject2D* other);
};
