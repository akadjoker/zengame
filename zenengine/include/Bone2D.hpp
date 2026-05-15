#pragma once

#include "Node2D.hpp"
#include <raylib.h>

// ----------------------------------------------------------------------------
// Bone2D — a single bone in a skeleton hierarchy.
//
// Bones ARE Node2D nodes — the engine's transform system handles parent→child
// inheritance automatically. Attach sprites/meshes as children of bones.
//
// Usage:
//   auto* root_bone = tree.create<Bone2D>(skeleton, "Hips");
//   auto* thigh     = tree.create<Bone2D>(root_bone, "Thigh_L");
//   thigh->length   = 60.f;
//   thigh->rotation = -10.f;
//
// The debug bone shape (diamond + line) is drawn when show_bone is true.
// In release builds set show_bone = false for all bones.
// ----------------------------------------------------------------------------

class Bone2D : public Node2D
{
public:
    explicit Bone2D(const std::string& p_name = "Bone2D");

    // ── Properties ───────────────────────────────────────────────────────────
    float length     = 50.0f;   // visual length in local-space pixels
    float thickness  = 8.0f;    // debug shape width at root
    Color bone_color = {180, 120, 60, 200};
    bool  show_bone  = true;    // draw debug shape

    // Rest pose (snapshot of the default transform).
    // Call save_rest_pose() after positioning bones in the editor.
    // Call reset_to_rest()  to restore them.
    Vec2  rest_position;
    float rest_rotation = 0.0f;
    Vec2  rest_scale    = {1.0f, 1.0f};

    void save_rest_pose();
    void reset_to_rest();

    // ── Node2D override ───────────────────────────────────────────────────────
    void _draw() override;
    void _draw_overlay();           // bone shape drawn on top of children
    void propagate_draw() override; // custom order: children first, bone on top
};
