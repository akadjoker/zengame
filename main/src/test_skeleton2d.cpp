// test_skeleton2d.cpp
//
// Interactive demo of Bone2D + Skeleton2D.
//
// Screen layout:
//   LEFT  — simple stick-figure (torso, arms, legs) with "walk" + "idle" clips
//   RIGHT — bare bone hierarchy debug view, scrubbing with mouse wheel
//
// Controls:
//   1          — play "walk" animation
//   2          — play "idle" animation
//   3          — play "attack" animation
//   SPACE      — pause / resume
//   R          — reset to rest pose
//   UP/DOWN    — speed  +/-
//   LEFT/RIGHT — seek manually (when paused)
//   B          — toggle bone debug shapes

#include "pch.hpp"
#include <raylib.h>
#include "SceneTree.hpp"
#include "View2D.hpp"
#include "Bone2D.hpp"
#include "Skeleton2D.hpp"
#include "Sprite2D.hpp"
#include "Rect2D.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static BoneAnimationClip make_walk_clip()
{
    BoneAnimationClip clip;
    clip.name     = "walk";
    clip.duration = 0.6f;
    clip.loop     = true;

    auto add_rot = [&](const char* bone, float a, float b)
    {
        BoneTrack t;
        t.bone_name     = bone;
        t.property      = BoneTrack::ROTATION;
        t.interpolation = BoneTrack::LINEAR;
        t.add_keyframe(0.0f,           a);
        t.add_keyframe(clip.duration * 0.5f, b);
        t.add_keyframe(clip.duration,  a);
        clip.tracks.push_back(t);
    };

    // Thigh rest = 180° (points down). Swing ±25° around that.
    add_rot("Thigh_L", 180.f - 25.f, 180.f + 25.f);
    add_rot("Shin_L",  180.f,         180.f + 30.f);   // knee only bends forward
    add_rot("Thigh_R", 180.f + 25.f, 180.f - 25.f);
    add_rot("Shin_R",  180.f + 30.f,  180.f);

    // Upper_L rest = 155°, Upper_R rest = -155°. Swing ±20°.
    add_rot("Upper_L", 155.f + 20.f, 155.f - 20.f);
    add_rot("Lower_L", 155.f,         155.f + 15.f);
    add_rot("Upper_R", -155.f - 20.f, -155.f + 20.f);
    add_rot("Lower_R", -155.f + 15.f, -155.f);

    // Slight torso sway
    add_rot("Spine",  -2.f,  2.f);

    return clip;
}

static BoneAnimationClip make_idle_clip()
{
    BoneAnimationClip clip;
    clip.name     = "idle";
    clip.duration = 1.2f;
    clip.loop     = true;

    // Gentle breathing — torso and head bob
    auto add_rot = [&](const char* bone, float a, float b)
    {
        BoneTrack t;
        t.bone_name = bone; t.property = BoneTrack::ROTATION;
        t.interpolation = BoneTrack::LINEAR;
        t.add_keyframe(0.0f, a);
        t.add_keyframe(clip.duration * 0.5f, b);
        t.add_keyframe(clip.duration, a);
        clip.tracks.push_back(t);
    };
    auto add_pos = [&](const char* bone, BoneTrack::Property prop, float a, float b)
    {
        BoneTrack t;
        t.bone_name = bone; t.property = prop;
        t.interpolation = BoneTrack::LINEAR;
        t.add_keyframe(0.0f, a);
        t.add_keyframe(clip.duration * 0.5f, b);
        t.add_keyframe(clip.duration, a);
        clip.tracks.push_back(t);
    };

    add_rot("Spine", -2.f, 2.f);
    add_rot("Head",  -3.f, 3.f);
    add_pos("Spine", BoneTrack::POS_Y, 0.f, -4.f);

    return clip;
}

static BoneAnimationClip make_attack_clip()
{
    BoneAnimationClip clip;
    clip.name     = "attack";
    clip.duration = 0.5f;
    clip.loop     = false;

    auto add_rot = [&](const char* bone,
                       float t0, float v0,
                       float t1, float v1,
                       float t2, float v2)
    {
        BoneTrack t;
        t.bone_name = bone; t.property = BoneTrack::ROTATION;
        t.interpolation = BoneTrack::LINEAR;
        t.add_keyframe(t0, v0);
        t.add_keyframe(t1, v1);
        t.add_keyframe(t2, v2);
        clip.tracks.push_back(t);
    };

    // Attack: right arm swings up to strike then returns. Absolute angles.
    // Upper_R rest = -155°. Swing up to -30° (arm raised), snap back.
    add_rot("Upper_R",  0.f, -155.f,  0.15f, -30.f,  0.5f, -155.f);
    add_rot("Lower_R",  0.f,  -155.f, 0.15f, -60.f,  0.5f, -155.f);
    add_rot("Spine",    0.f,  0.f,    0.1f,  -15.f,  0.5f,   0.f);

    return clip;
}

// ─────────────────────────────────────────────────────────────────────────────
// Build skeleton hierarchy
// ─────────────────────────────────────────────────────────────────────────────

// Helper — bone attached at tip of parent (tip is at local {0,-len} but after
// parent rotation the actual offset becomes rotate(parent_rot, {0,-len})).
// We just pass the parent length so callers don't have to repeat it.
static Bone2D* bone_at_tip(SceneTree& tree, Bone2D* parent_bone,
                            const char* name,
                            float length, float thickness,
                            float local_rot = 0.f,
                            Color c = {180, 120, 60, 220})
{
    auto* b = tree.create<Bone2D>(parent_bone, name);
    // child sits at parent tip in parent-local space = {0, -parent.length}
    b->position  = {0.f, -parent_bone->length};
    b->length    = length;
    b->thickness = thickness;
    b->rotation  = local_rot;
    b->bone_color = c;
    b->save_rest_pose();
    return b;
}

static Skeleton2D* build_skeleton(SceneTree& tree, Node* parent, const char* name)
{
    auto* skel = tree.create<Skeleton2D>(parent, name);

    // ── Coordinate convention ──────────────────────────────────────────────
    // Bone tip   = {0, -length} in bone-local (points UP when rotation=0).
    // rotation=0   → bone points UP   (use for spine, head)
    // rotation=180 → bone points DOWN (use for legs)
    // rotation=155 → bone points down-left  (left arm hanging)
    // rotation=-155→ bone points down-right (right arm hanging)
    //
    // skel->position = hip centre.  Legs go down from there, torso goes up.
    // ──────────────────────────────────────────────────────────────────────

    const Color torso_c = {180, 120,  60, 220};
    const Color leg_c   = {140, 100,  50, 220};
    const Color arm_c   = {160, 110,  55, 220};
    const Color head_c  = {210, 170, 110, 230};

    // ── Hips (tiny, just a pivot visual) ──
    auto* hips = tree.create<Bone2D>(skel, "Hips");
    hips->position  = {0.f, 0.f};   // skel.position IS the hip centre
    hips->length    = 16.f;
    hips->thickness = 14.f;
    hips->rotation  = 0.f;          // short nub pointing up
    hips->bone_color = torso_c;
    hips->save_rest_pose();

    // ── Spine ──
    auto* spine = bone_at_tip(tree, hips, "Spine", 70.f, 16.f, 0.f, torso_c);

    // ── Head ──
    bone_at_tip(tree, spine, "Head", 32.f, 20.f, 0.f, head_c);

    // ── Arms: attached near the shoulder (80% up the spine) ──
    // We can't use bone_at_tip here because arms don't start at spine tip.
    // Instead: shoulder is ~10px below spine tip → position = {0, -60}
    auto* upper_l = tree.create<Bone2D>(spine, "Upper_L");
    upper_l->position  = {0.f, -60.f};  // shoulder in spine-local
    upper_l->length    = 40.f;
    upper_l->thickness = 10.f;
    upper_l->rotation  = 155.f;   // points down-left (hanging arm)
    upper_l->bone_color = arm_c;
    upper_l->save_rest_pose();

    bone_at_tip(tree, upper_l, "Lower_L", 36.f, 8.f, 0.f, arm_c);

    auto* upper_r = tree.create<Bone2D>(spine, "Upper_R");
    upper_r->position  = {0.f, -60.f};  // same shoulder height
    upper_r->length    = 40.f;
    upper_r->thickness = 10.f;
    upper_r->rotation  = -155.f;  // points down-right
    upper_r->bone_color = arm_c;
    upper_r->save_rest_pose();

    bone_at_tip(tree, upper_r, "Lower_R", 36.f, 8.f, 0.f, arm_c);

    // ── Left leg ── (root = hip centre, offset left, points down)
    auto* thigh_l = tree.create<Bone2D>(hips, "Thigh_L");
    thigh_l->position  = {-14.f, 0.f};
    thigh_l->length    = 55.f;
    thigh_l->thickness = 13.f;
    thigh_l->rotation  = 180.f;   // points DOWN
    thigh_l->bone_color = leg_c;
    thigh_l->save_rest_pose();

    bone_at_tip(tree, thigh_l, "Shin_L", 50.f, 10.f, 0.f, leg_c);

    // ── Right leg ──
    auto* thigh_r = tree.create<Bone2D>(hips, "Thigh_R");
    thigh_r->position  = {14.f, 0.f};
    thigh_r->length    = 55.f;
    thigh_r->thickness = 13.f;
    thigh_r->rotation  = 180.f;
    thigh_r->bone_color = leg_c;
    thigh_r->save_rest_pose();

    bone_at_tip(tree, thigh_r, "Shin_R", 50.f, 10.f, 0.f, leg_c);

    // ── Register clips ──
    skel->add_clip(make_walk_clip());
    skel->add_clip(make_idle_clip());
    skel->add_clip(make_attack_clip());
    skel->play("idle");

    return skel;
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1200, 700, "ZenEngine – Skeleton2D Demo");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    SceneTree tree;

    // ── Scene root ──
    auto* root = new Node2D("Root");

    auto* cam = new View2D("Camera");
    cam->is_current = true;
    cam->zoom       = 1.0f;
    cam->position   = Vec2(600.f, 350.f);  // world-coords = screen-coords
    root->add_child(cam);

    // hip y = ground(510) - leg_total(105) = 405
    // LEFT skeleton
    Skeleton2D* skel_a = build_skeleton(tree, root, "SkelA");
    skel_a->position = {300.f, 405.f};

    // RIGHT skeleton (mirrored)
    Skeleton2D* skel_b = build_skeleton(tree, root, "SkelB");
    skel_b->position = {900.f, 405.f};
    skel_b->scale    = {-1.f, 1.f};
    //skel_b->play("walk");

    tree.set_root(root);
    tree.start();

    bool bones_visible  = true;
    float manual_seek   = 0.0f;

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        tree.process(dt);

        // ── Input ──────────────────────────────────────────────────────────
        if (IsKeyPressed(KEY_ONE))    { skel_a->play("walk");   skel_b->play("walk"); }
        if (IsKeyPressed(KEY_TWO))    { skel_a->play("idle");   skel_b->play("idle"); }
        if (IsKeyPressed(KEY_THREE))  { skel_a->play("attack"); skel_b->play("attack"); }
        if (IsKeyPressed(KEY_SPACE))
        {
            skel_a->is_playing() ? skel_a->pause() : skel_a->resume();
            skel_b->is_playing() ? skel_b->pause() : skel_b->resume();
        }
        if (IsKeyPressed(KEY_R))      { skel_a->stop(); skel_b->stop(); }
        if (IsKeyPressed(KEY_B))
        {
            bones_visible = !bones_visible;
            // Toggle debug on every Bone2D by iterating the group
            std::vector<Node*> all_bones;
            tree.get_nodes_in_group("__bones__", all_bones);
            // Simple: just recurse manually
            auto toggle = [&](auto& self, Node* n) -> void {
                if (n->is_a(NodeType::Bone2D))
                    static_cast<Bone2D*>(n)->show_bone = bones_visible;
                for (size_t i = 0; i < n->get_child_count(); ++i)
                    self(self, n->get_child(i));
            };
            toggle(toggle, root);
        }

        if (IsKeyDown(KEY_UP))   { skel_a->speed += dt; skel_b->speed = skel_a->speed; }
        if (IsKeyDown(KEY_DOWN)) { skel_a->speed -= dt; skel_b->speed = skel_a->speed; }
        skel_a->speed = std::clamp(skel_a->speed, -3.0f, 3.0f);
        skel_b->speed = skel_a->speed;

        if (!skel_a->is_playing())
        {
            if (IsKeyDown(KEY_RIGHT)) manual_seek += dt * 2.0f;
            if (IsKeyDown(KEY_LEFT))  manual_seek -= dt * 2.0f;
            skel_a->seek(manual_seek);
            skel_b->seek(manual_seek);
        }
        else
        {
            manual_seek = skel_a->get_current_time();
        }

        // ── Draw ───────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground({20, 20, 30, 255});

        tree.draw();

        // ── Ground lines ──
        DrawLineEx({50.f, 510.f},  {550.f, 510.f},  2.f, {100, 100, 120, 180});
        DrawLineEx({650.f, 510.f}, {1150.f, 510.f}, 2.f, {100, 100, 120, 180});

        // ── HUD ───────────────────────────────────────────────────────────
        const char* anim_name = skel_a->current_animation().c_str();
        const float cur_t     = skel_a->get_current_time();
        const BoneAnimationClip* clip = skel_a->get_clip(anim_name);
        const float dur = clip ? clip->duration : 1.0f;

        DrawRectangle(10, 10, 340, 120, {0, 0, 0, 160});

        DrawText(TextFormat("Animation : %s", anim_name),     18, 18, 18, WHITE);
        DrawText(TextFormat("Time      : %.2f / %.2f", cur_t, dur), 18, 40, 18, WHITE);
        DrawText(TextFormat("Speed     : %.2f", skel_a->speed),     18, 62, 18, WHITE);
        DrawText(skel_a->is_playing() ? "State     : PLAYING" : "State     : PAUSED",
                 18, 84, 18, skel_a->is_playing() ? GREEN : ORANGE);

        // Progress bar
        const int bx = 18, by = 108, bw = 320, bh = 8;
        DrawRectangle(bx, by, bw, bh, {60, 60, 60, 200});
        DrawRectangle(bx, by, (int)(bw * (cur_t / dur)), bh, {80, 180, 255, 220});

        // Controls hint
        DrawText("[1]walk [2]idle [3]attack  [SPACE]pause  [R]stop", 10, GetScreenHeight()-44, 16, {180,180,180,200});
        DrawText("[UP/DN]speed  [LEFT/RIGHT]seek(paused)  [B]bones", 10, GetScreenHeight()-22, 16, {180,180,180,200});

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
