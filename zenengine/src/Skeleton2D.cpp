#include "Skeleton2D.hpp"
#include "Bone2D.hpp"

// ── BoneTrack ─────────────────────────────────────────────────────────────────

void BoneTrack::add_keyframe(float time, float value)
{
    // Insert sorted by time
    BoneKeyframe kf;
    kf.time  = time;
    kf.value = value;

    for (auto it = keyframes.begin(); it != keyframes.end(); ++it)
    {
        if (time < it->time)
        {
            keyframes.insert(it, kf);
            return;
        }
    }
    keyframes.push_back(kf);
}

float BoneTrack::sample(float time) const
{
    if (keyframes.empty())  return 0.0f;
    if (keyframes.size() == 1) return keyframes[0].value;

    // Clamp to range
    if (time <= keyframes.front().time) return keyframes.front().value;
    if (time >= keyframes.back().time)  return keyframes.back().value;

    // Find surrounding pair
    for (size_t i = 0; i + 1 < keyframes.size(); ++i)
    {
        const BoneKeyframe& a = keyframes[i];
        const BoneKeyframe& b = keyframes[i + 1];

        if (time >= a.time && time <= b.time)
        {
            if (interpolation == STEP)
                return a.value;

            const float span = b.time - a.time;
            if (span < 1e-6f) return a.value;

            float t = (time - a.time) / span;

            // For rotation: pick shortest path around 360°
            if (property == ROTATION)
            {
                float diff = b.value - a.value;
                while (diff >  180.0f) diff -= 360.0f;
                while (diff < -180.0f) diff += 360.0f;
                return a.value + t * diff;
            }

            return a.value + t * (b.value - a.value);
        }
    }
    return keyframes.back().value;
}

// ── Skeleton2D ────────────────────────────────────────────────────────────────

Skeleton2D::Skeleton2D(const std::string& p_name) : Node2D(p_name)
{
    node_type = NodeType::Skeleton2D;
}

// ── Clip library ──────────────────────────────────────────────────────────────

void Skeleton2D::add_clip(const BoneAnimationClip& clip)
{
    // Replace if already exists
    for (auto& c : m_clips)
    {
        if (c.name == clip.name) { c = clip; return; }
    }
    m_clips.push_back(clip);
}

void Skeleton2D::remove_clip(const std::string& name)
{
    m_clips.erase(
        std::remove_if(m_clips.begin(), m_clips.end(),
                       [&](const BoneAnimationClip& c){ return c.name == name; }),
        m_clips.end());
}

BoneAnimationClip* Skeleton2D::get_clip(const std::string& name)
{
    for (auto& c : m_clips)
        if (c.name == name) return &c;
    return nullptr;
}

const BoneAnimationClip* Skeleton2D::get_clip(const std::string& name) const
{
    for (const auto& c : m_clips)
        if (c.name == name) return &c;
    return nullptr;
}

std::vector<std::string> Skeleton2D::get_clip_names() const
{
    std::vector<std::string> names;
    names.reserve(m_clips.size());
    for (const auto& c : m_clips)
        names.push_back(c.name);
    return names;
}

// ── Playback ──────────────────────────────────────────────────────────────────

void Skeleton2D::play(const std::string& name, bool loop, float p_speed)
{
    const BoneAnimationClip* clip = get_clip(name);
    if (!clip) return;

    // Reset all bones to rest before starting a new clip so that bones not
    // covered by this clip's tracks don't inherit pose from the previous clip.
    reset_to_rest();

    m_current    = name;
    m_time       = 0.0f;
    m_playing    = true;
    speed        = p_speed;

    // Allow caller to override loop per-play
    // (clip->loop is the default; we don't mutate the clip)
    apply_clip(*clip);
}

void Skeleton2D::stop()
{
    m_playing = false;
    m_time    = 0.0f;
    reset_to_rest();
}

void Skeleton2D::pause()  { m_playing = false; }
void Skeleton2D::resume() { m_playing = true;  }

void Skeleton2D::seek(float time)
{
    const BoneAnimationClip* clip = get_clip(m_current);
    if (!clip) return;
    m_time = std::clamp(time, 0.0f, clip->duration);
    apply_clip(*clip);
}

// ── Update ────────────────────────────────────────────────────────────────────

void Skeleton2D::_update(float dt)
{
    Node2D::_update(dt);

    if (!m_playing || m_current.empty()) return;

    const BoneAnimationClip* clip = get_clip(m_current);
    if (!clip) return;

    m_time += dt * speed;

    bool did_loop = false;

    if (speed >= 0.0f)
    {
        if (m_time >= clip->duration)
        {
            if (clip->loop)
            {
                m_time   = fmodf(m_time, clip->duration);
                did_loop = true;
            }
            else
            {
                m_time    = clip->duration;
                m_playing = false;
                apply_clip(*clip);
                if (on_animation_finished) on_animation_finished(m_current);
                return;
            }
        }
    }
    else  // reverse
    {
        if (m_time < 0.0f)
        {
            if (clip->loop)
            {
                m_time   = clip->duration + fmodf(m_time, clip->duration);
                did_loop = true;
            }
            else
            {
                m_time    = 0.0f;
                m_playing = false;
                apply_clip(*clip);
                if (on_animation_finished) on_animation_finished(m_current);
                return;
            }
        }
    }

    apply_clip(*clip);

    if (did_loop && on_animation_looped)
        on_animation_looped(m_current);
}

// ── Internal: apply all tracks of a clip at current time ─────────────────────

void Skeleton2D::apply_clip(const BoneAnimationClip& clip)
{
    for (const BoneTrack& track : clip.tracks)
    {
        Bone2D* bone = find_bone(track.bone_name);
        if (!bone) continue;

        const float v = track.sample(m_time);

        switch (track.property)
        {
        case BoneTrack::POS_X:    bone->position.x = bone->rest_position.x + v; break;
        case BoneTrack::POS_Y:    bone->position.y = bone->rest_position.y + v; break;
        case BoneTrack::ROTATION: bone->rotation   = bone->rest_rotation   + v; break;
        case BoneTrack::SCALE_X:  bone->scale.x    = bone->rest_scale.x    * v; break;
        case BoneTrack::SCALE_Y:  bone->scale.y    = bone->rest_scale.y    * v; break;
        }
        bone->invalidate_transform();
    }
}

// ── Bone lookup ───────────────────────────────────────────────────────────────

static Bone2D* find_bone_recursive(Node* node, const std::string& bone_name)
{
    if (!node) return nullptr;
    if (node->is_a(NodeType::Bone2D) && node->name == bone_name)
        return static_cast<Bone2D*>(node);
    for (size_t i = 0; i < node->get_child_count(); ++i)
    {
        Bone2D* result = find_bone_recursive(node->get_child(i), bone_name);
        if (result) return result;
    }
    return nullptr;
}

Bone2D* Skeleton2D::find_bone(const std::string& bone_name) const
{
    for (size_t i = 0; i < get_child_count(); ++i)
    {
        Bone2D* result = find_bone_recursive(get_child(i), bone_name);
        if (result) return result;
    }
    return nullptr;
}

// ── Reset to rest ─────────────────────────────────────────────────────────────

static void reset_bones_recursive(Node* node)
{
    if (!node) return;
    if (node->is_a(NodeType::Bone2D))
        static_cast<Bone2D*>(node)->reset_to_rest();
    for (size_t i = 0; i < node->get_child_count(); ++i)
        reset_bones_recursive(node->get_child(i));
}

void Skeleton2D::reset_to_rest()
{
    for (size_t i = 0; i < get_child_count(); ++i)
        reset_bones_recursive(get_child(i));
}
