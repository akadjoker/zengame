#include "pch.hpp"
#include "CollisionObject2D.hpp"
#include "Collider2D.hpp"

static Collider2D* FindColliderRecursive(Node* node)
{
    if (!node)
    {
        return nullptr;
    }

    if (auto* c = dynamic_cast<Collider2D*>(node))
    {
        return c;
    }

    for (size_t i = 0; i < node->get_child_count(); ++i)
    {
        if (Collider2D* found = FindColliderRecursive(node->get_child(i)))
        {
            return found;
        }
    }
    return nullptr;
}

static void CollectCollidersRecursive(Node* node, std::vector<Collider2D*>& out)
{
    if (!node)
    {
        return;
    }

    if (auto* c = dynamic_cast<Collider2D*>(node))
    {
        out.push_back(c);
    }

    for (size_t i = 0; i < node->get_child_count(); ++i)
    {
        CollectCollidersRecursive(node->get_child(i), out);
    }
}

CollisionObject2D::CollisionObject2D(const std::string& p_name)
    : Node2D(p_name)
{
}

void CollisionObject2D::set_collision_layer_bit(int bit)
{
    if (bit < 0 || bit >= 32) return;
    collision_layer = (1u << bit);
}

void CollisionObject2D::set_collision_mask_bit(int bit)
{
    if (bit < 0 || bit >= 32) return;
    collision_mask = (1u << bit);
}

void CollisionObject2D::add_collision_mask_bit(int bit)
{
    if (bit < 0 || bit >= 32) return;
    collision_mask |= (1u << bit);
}

void CollisionObject2D::remove_collision_mask_bit(int bit)
{
    if (bit < 0 || bit >= 32) return;
    collision_mask &= ~(1u << bit);
}

Collider2D* CollisionObject2D::get_collider() const
{
    return FindColliderRecursive(const_cast<CollisionObject2D*>(this));
}

void CollisionObject2D::get_colliders(std::vector<Collider2D*>& out) const
{
    out.clear();
    CollectCollidersRecursive(const_cast<CollisionObject2D*>(this), out);
}

void CollisionObject2D::on_collision_enter(CollisionObject2D* /*other*/)
{
}

void CollisionObject2D::on_collision_stay(CollisionObject2D* /*other*/)
{
}

void CollisionObject2D::on_collision_exit(CollisionObject2D* /*other*/)
{
}
