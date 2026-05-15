#pragma once

#include "Node2D.hpp"
#include <string>
#include <vector>
#include <functional>

// ----------------------------------------------------------------------------
// Skeleton2D — root node of a bone hierarchy; owns animation clips and drives
// playback by writing bone transforms each frame.
//
// Architecture:
//   Skeleton2D (root)
//     └─ Bone2D "Hips"
//          ├─ Bone2D "Spine"
//          │    └─ Bone2D "Head"
//          ├─ Bone2D "Thigh_L"
//          │    └─ Bone2D "Shin_L"
//          └─ Bone2D "Thigh_R"
//
// Clips store keyframe tracks per-bone by name. The update loop samples each
// track and writes position/rotation/scale directly onto the matching Bone2D.
//
// Usage:
//   auto* skel = tree.create<Skeleton2D>(root, "Player");
//   // ... build bone hierarchy as children ...
//
//   // Build an animation in code (or load from file):
//   BoneAnimationClip run;
//   run.name     = "run";
//   run.duration = 0.6f;
//   BoneTrack t; t.bone_name = "Thigh_L"; t.property = BoneTrack::ROTATION;
//   t.add_keyframe(0.0f,  0.f, BoneTrack::LINEAR);
//   t.add_keyframe(0.3f, 40.f, BoneTrack::LINEAR);
//   t.add_keyframe(0.6f,  0.f, BoneTrack::LINEAR);
//   run.tracks.push_back(t);
//   skel->add_clip(run);
//   skel->play("run");
// ----------------------------------------------------------------------------

struct BoneKeyframe
{
    float time  = 0.0f;
    float value = 0.0f;
};

struct BoneTrack
{
    enum Property : uint8_t { POS_X, POS_Y, ROTATION, SCALE_X, SCALE_Y };
    enum Interpolation : uint8_t { LINEAR, STEP };

    std::string   bone_name;
    Property      property     = ROTATION;
    Interpolation interpolation = LINEAR;

    std::vector<BoneKeyframe> keyframes;

    // Add a keyframe at the given time. Keyframes are kept sorted by time.
    void add_keyframe(float time, float value);

    // Sample the track at a given time (does NOT clamp to duration — caller handles wrapping).
    float sample(float time) const;
};

struct BoneAnimationClip
{
    std::string              name;
    float                    duration = 1.0f;
    bool                     loop     = true;
    std::vector<BoneTrack>   tracks;
};

// ---------------------------------------------------------------------------

class Bone2D;   // forward

class Skeleton2D : public Node2D
{
public:
    explicit Skeleton2D(const std::string& p_name = "Skeleton2D");

    // ── Metadata (set by SkeletonLoader) ─────────────────────────────────────
    std::string atlas_texture;  // e.g. "spineboy.png"
    std::string source_path;    // e.g. "assets/spine/spineboy.zenskel"
    // ── Animation library ────────────────────────────────────────────────────
    void add_clip   (const BoneAnimationClip& clip);
    void remove_clip(const std::string& name);
    BoneAnimationClip*       get_clip(const std::string& name);
    const BoneAnimationClip* get_clip(const std::string& name) const;
    std::vector<std::string> get_clip_names() const;

    // ── Playback ─────────────────────────────────────────────────────────────
    void  play  (const std::string& name, bool loop = true, float speed = 1.0f);
    void  stop  ();
    void  pause ();
    void  resume();
    void  seek  (float time);
    float get_current_time() const { return m_time; }
    bool  is_playing()       const { return m_playing; }
    const std::string& current_animation() const { return m_current; }

    // Speed multiplier (1.0 = normal, 2.0 = double speed, -1.0 = reverse)
    float speed = 1.0f;

    // ── Callbacks ────────────────────────────────────────────────────────────
    std::function<void(const std::string&)> on_animation_finished;
    std::function<void(const std::string&)> on_animation_looped;

    // ── Bone access ──────────────────────────────────────────────────────────
    // Find a Bone2D by name anywhere in the skeleton hierarchy.
    Bone2D* find_bone(const std::string& bone_name) const;

    // Reset all bone transforms to their saved rest pose.
    void reset_to_rest();

    // ── Node2D override ───────────────────────────────────────────────────────
    void _update(float dt) override;

private:
    std::vector<BoneAnimationClip> m_clips;
    std::string                    m_current;
    float                          m_time    = 0.0f;
    bool                           m_playing = false;

    void apply_clip(const BoneAnimationClip& clip);
};
