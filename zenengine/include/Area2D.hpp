#pragma once

#include "CollisionObject2D.hpp"
#include <functional>

// ----------------------------------------------------------------------------
// Area2D
//
// Trigger zone that detects when bodies enter/exit its collider.
// Does NOT push bodies — resolves as a sensor (is_trigger = true).
//
// Usage:
//   auto* area = tree.create<Area2D>(root, "Zone");
//   area->on_body_entered = [](CollisionObject2D* body) { ... };
//   area->on_body_exited  = [](CollisionObject2D* body) { ... };
// ----------------------------------------------------------------------------
class Area2D : public CollisionObject2D
{
public:
    explicit Area2D(const std::string& p_name = "Area2D")
        : CollisionObject2D(p_name)
    {
        is_trigger = true;
    }

    // Fired when a body first overlaps this area.
    std::function<void(CollisionObject2D*)> on_body_entered;
    // Fired when a body stops overlapping this area.
    std::function<void(CollisionObject2D*)> on_body_exited;

    void on_collision_enter(CollisionObject2D* other) override
    {
        if (on_body_entered) on_body_entered(other);
    }

    void on_collision_exit(CollisionObject2D* other) override
    {
        if (on_body_exited) on_body_exited(other);
    }
};

