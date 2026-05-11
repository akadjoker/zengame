#include "pch.hpp"
#include "CharacterBody2D.hpp"
#include "Collider2D.hpp"
#include "Collision2D.hpp"
#include "SceneTree.hpp"
#include "TileMap2D.hpp"

namespace
{
void GatherNearbyBodies(CharacterBody2D* self, const Rectangle& area, std::vector<CollisionObject2D*>& out)
{
    out.clear();
    SceneTree* tree = self->get_tree();
    if (!tree)
    {
        return;
    }
    tree->query_collision_candidates(area, self, out);
}

void CollectTileMaps(Node* node, std::vector<TileMap2D*>& out)
{
    if (!node)
    {
        return;
    }
    if (node->is_a(NodeType::TileMap2D))
    {
        out.push_back(static_cast<TileMap2D*>(node));
    }
    for (size_t i = 0; i < node->get_child_count(); ++i)
    {
        CollectTileMaps(node->get_child(i), out);
    }
}

bool CollideWithSolidTiles(Node* root, const Rectangle& box)
{
    if (!root)
    {
        return false;
    }

    std::vector<TileMap2D*> maps;
    maps.reserve(8);
    CollectTileMaps(root, maps);

    for (TileMap2D* tm : maps)
    {
        if (!tm || !tm->visible || tm->width <= 0 || tm->height <= 0)
        {
            continue;
        }

        int gx0 = 0, gy0 = 0, gx1 = 0, gy1 = 0;
        tm->world_to_grid(Vec2(box.x, box.y), gx0, gy0);
        tm->world_to_grid(Vec2(box.x + box.width, box.y + box.height), gx1, gy1);

        gx0 = std::max(0, std::min(gx0, tm->width - 1));
        gy0 = std::max(0, std::min(gy0, tm->height - 1));
        gx1 = std::max(0, std::min(gx1, tm->width - 1));
        gy1 = std::max(0, std::min(gy1, tm->height - 1));
        if (gx1 < gx0) std::swap(gx0, gx1);
        if (gy1 < gy0) std::swap(gy0, gy1);

        for (int gy = gy0; gy <= gy1; ++gy)
        {
            for (int gx = gx0; gx <= gx1; ++gx)
            {
                if (!tm->is_solid_cell(gx, gy))
                {
                    continue;
                }
                const Rectangle r = tm->cell_rect_world(gx, gy);
                if (CheckCollisionRecs(box, r))
                {
                    return true;
                }
            }
        }
    }

    return false;
}
}

CharacterBody2D::CharacterBody2D(const std::string& p_name)
    : CollisionObject2D(p_name)
{
    node_type = NodeType::CharacterBody2D;
    is_trigger = false;
}

void CharacterBody2D::move_topdown(const Vec2& velocity, float dt)
{
    const float dx = velocity.x * dt;
    const float dy = velocity.y * dt;

    const int steps_x = static_cast<int>(std::fabs(dx));
    const int steps_y = static_cast<int>(std::fabs(dy));

    const int sx = (dx > 0.0f) - (dx < 0.0f);
    const int sy = (dy > 0.0f) - (dy < 0.0f);

    for (int i = 0; i < steps_x; ++i)
    {
        if (place_free(position.x + static_cast<float>(sx), position.y))
        {
            position.x += static_cast<float>(sx);
        }
        else
        {
            break;
        }
    }

    for (int i = 0; i < steps_y; ++i)
    {
        if (place_free(position.x, position.y + static_cast<float>(sy)))
        {
            position.y += static_cast<float>(sy);
        }
        else
        {
            break;
        }
    }

}

bool CharacterBody2D::place_free(float x, float y)
{
    Collider2D* self_col = get_collider();
    if (!self_col || !enabled || !active)
    {
        return true;
    }

    const Vec2 old_pos = position;
    position = Vec2(x, y);
    invalidate_transform();
    const Rectangle self_aabb = self_col->get_world_aabb();

    if (CollideWithSolidTiles(get_root(), self_aabb))
    {
        position = old_pos;
        invalidate_transform();
        return false;
    }

    std::vector<CollisionObject2D*> nearby;
    GatherNearbyBodies(this, self_aabb, nearby);

    bool free = true;
    std::vector<Collider2D*> other_cols;
    for (CollisionObject2D* other : nearby)
    {
        if (!Collision2D::CanCollide(collision_layer, collision_mask, other->collision_layer, other->collision_mask))
        {
            continue;
        }
        other->get_colliders(other_cols);
        Contact2D contact;
        for (Collider2D* other_col : other_cols)
        {
            if (other_col && Collision2D::Collide(*self_col, *other_col, &contact))
            {
                free = false;
                break;
            }
        }
        if (!free) break;
    }

    position = old_pos;
    invalidate_transform();
    return free;
}

CollisionObject2D* CharacterBody2D::place_meeting(float x, float y)
{
    Collider2D* self_col = get_collider();
    if (!self_col || !enabled || !active)
    {
        return nullptr;
    }

    const Vec2 old_pos = position;
    position = Vec2(x, y);
    invalidate_transform();
    const Rectangle self_aabb = self_col->get_world_aabb();

    if (CollideWithSolidTiles(get_root(), self_aabb))
    {
        position = old_pos;
        invalidate_transform();
        return nullptr;
    }

    std::vector<CollisionObject2D*> nearby;
    GatherNearbyBodies(this, self_aabb, nearby);

    CollisionObject2D* hit = nullptr;
    std::vector<Collider2D*> other_cols;
    for (CollisionObject2D* other : nearby)
    {
        if (!Collision2D::CanCollide(collision_layer, collision_mask, other->collision_layer, other->collision_mask))
        {
            continue;
        }
        other->get_colliders(other_cols);
        Contact2D contact;
        for (Collider2D* other_col : other_cols)
        {
            if (other_col && Collision2D::Collide(*self_col, *other_col, &contact))
            {
                hit = other;
                break;
            }
        }
        if (hit) break;
    }

    position = old_pos;
    invalidate_transform();
    return hit;
}

bool CharacterBody2D::move_and_collide(const Vec2& motion, CollisionInfo2D* result)
{
    return move_and_collide(motion.x, motion.y, result);
}

bool CharacterBody2D::move_and_collide(float vel_x, float vel_y, CollisionInfo2D* result)
{
    Collider2D* self_col = get_collider();
    if (!self_col || !enabled || !active)
    {
        position.x += vel_x;
        position.y += vel_y;
        return false;
    }

    const float skin = 0.05f;
    const Rectangle start_aabb = self_col->get_world_aabb();

    Rectangle move_aabb = start_aabb;
    if (vel_x < 0.0f) move_aabb.x += vel_x;
    if (vel_y < 0.0f) move_aabb.y += vel_y;
    move_aabb.width += std::fabs(vel_x);
    move_aabb.height += std::fabs(vel_y);
    move_aabb.x -= 2.0f;
    move_aabb.y -= 2.0f;
    move_aabb.width += 4.0f;
    move_aabb.height += 4.0f;

    std::vector<CollisionObject2D*> nearby;
    GatherNearbyBodies(this, move_aabb, nearby);

    position.x += vel_x;
    position.y += vel_y;
    invalidate_transform();
    if (CollideWithSolidTiles(get_root(), self_col->get_world_aabb()))
    {
        position.x -= vel_x;
        position.y -= vel_y;
        invalidate_transform();
        if (result)
        {
            result->hit = true;
            result->collider = nullptr;
            result->normal = Vec2();
            result->depth = 0.0f;
        }
        return true;
    }

    bool hit = false;
    float best_depth = std::numeric_limits<float>::max();
    Vec2 best_normal;
    CollisionObject2D* best_other = nullptr;

    std::vector<Collider2D*> other_cols;
    for (CollisionObject2D* other : nearby)
    {
        if (!Collision2D::CanCollide(collision_layer, collision_mask, other->collision_layer, other->collision_mask))
        {
            continue;
        }
        other->get_colliders(other_cols);
        for (Collider2D* other_col : other_cols)
        {
            if (!other_col) continue;
            Contact2D contact;
            if (Collision2D::Collide(*self_col, *other_col, &contact) &&
                contact.depth > 0.0f && contact.depth < best_depth)
            {
                hit = true;
                best_depth = contact.depth;
                best_normal = contact.normal;
                best_other = other;
            }
        }
    }

    if (!hit)
    {
        return false;
    }

    const Vec2 this_center = self_col->get_world_center();
    const Vec2 other_center = best_other->get_collider()->get_world_center();
    const Vec2 dir = this_center - other_center;
    if (dir.dot(best_normal) < 0.0f)
    {
        best_normal = -best_normal;
    }

    position += best_normal * (best_depth + skin);

    if (result)
    {
        result->collider = best_other;
        result->normal = best_normal;
        result->depth = best_depth;
        result->hit = true;
    }

    return true;
}

bool CharacterBody2D::move_and_stop(Vec2& velocity, float dt, CollisionInfo2D* result)
{
    CollisionInfo2D local_result;
    CollisionInfo2D* out = result ? result : &local_result;
    out->hit = false;
    out->collider = nullptr;
    out->normal = Vec2();
    out->depth = 0.0f;

    const Vec2 motion = velocity * dt;
    const bool hit = move_and_collide(motion.x, motion.y, out);
    if (hit)
    {
        velocity = Vec2();
    }
    return hit;
}

bool CharacterBody2D::snap_to_floor(float snap_len, const Vec2& up_direction, Vec2& velocity)
{
    const float vel_dot_up = velocity.dot(up_direction);
    if (vel_dot_up < 0.0f)
    {
        return false;
    }

    const Vec2 old_pos = position;
    CollisionInfo2D col;
    const float snap_x = -up_direction.x * snap_len;
    const float snap_y = -up_direction.y * snap_len;

    if (move_and_collide(snap_x, snap_y, &col))
    {
        const float dot_up = col.normal.dot(up_direction);
        if (dot_up > 0.7f)
        {
            on_floor = true;
            on_wall = false;
            on_ceiling = false;
            floor_normal = col.normal;

            if (vel_dot_up > 0.0f)
            {
                velocity -= up_direction * vel_dot_up;
            }
            return true;
        }
    }

    position = old_pos;
    return false;
}

bool CharacterBody2D::move_and_slide(Vec2& velocity, float dt, const Vec2& up_direction)
{
    on_floor = on_wall = on_ceiling = false;
    floor_normal = Vec2();

    if (!enabled || !active || !get_collider())
    {
        position += velocity * dt;
        return false;
    }

    const int max_slides = 4;
    const float skin = 0.05f;
    Vec2 motion = velocity * dt;

    for (int slide = 0; slide < max_slides; ++slide)
    {
        if (motion.dot(motion) < 1e-10f)
        {
            break;
        }

        const Vec2 old_pos = position;
        position += motion;
        invalidate_transform();

        CollisionInfo2D best;
        bool hit = false;
        float best_depth = std::numeric_limits<float>::max();

        Collider2D* self_col = get_collider();
        Rectangle query = self_col->get_world_aabb();
        query.x -= 2.0f; query.y -= 2.0f; query.width += 4.0f; query.height += 4.0f;

        std::vector<CollisionObject2D*> nearby;
        GatherNearbyBodies(this, query, nearby);

        std::vector<Collider2D*> other_cols;
        for (CollisionObject2D* other : nearby)
        {
            if (!Collision2D::CanCollide(collision_layer, collision_mask, other->collision_layer, other->collision_mask))
            {
                continue;
            }
            other->get_colliders(other_cols);
            for (Collider2D* other_col : other_cols)
            {
                if (!other_col) continue;
                Contact2D contact;
                if (Collision2D::Collide(*self_col, *other_col, &contact) &&
                    contact.depth > 0.0f && contact.depth < best_depth)
                {
                    hit = true;
                    best_depth = contact.depth;
                    best.collider = other;
                    best.normal = contact.normal;
                    best.depth = contact.depth;
                    best.hit = true;
                }
            }
        }

        if (!hit)
        {
            break;
        }

        const Vec2 this_center = self_col->get_world_center();
        const Vec2 other_center = best.collider->get_collider()->get_world_center();
        Vec2 normal = best.normal;
        if ((this_center - other_center).dot(normal) < 0.0f)
        {
            normal = -normal;
        }

        position += normal * (best.depth + skin);
        invalidate_transform();

        const float dot_up = normal.dot(up_direction);
        if (dot_up > 0.7f)
        {
            on_floor = true;
            floor_normal = normal;
        }
        else if (dot_up < -0.7f)
        {
            on_ceiling = true;
        }
        else
        {
            on_wall = true;
        }

        const Vec2 travel = position - old_pos;
        const Vec2 remainder = motion - travel;
        const float dot_r = remainder.dot(normal);
        motion = remainder - normal * dot_r;

        const float dot_v = velocity.dot(normal);
        velocity = velocity - normal * dot_v;
    }

    return (on_floor || on_wall || on_ceiling);
}
