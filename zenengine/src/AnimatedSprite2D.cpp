#include "AnimatedSprite2D.hpp"
#include "Assets.hpp"
#include "render.hpp"

AnimatedSprite2D::AnimatedSprite2D(const std::string& p_name)
    : Node2D(p_name)
{
    node_type = NodeType::AnimatedSprite2D;
}

// ----------------------------------------------------------------------------
// Animation library
// ----------------------------------------------------------------------------

void AnimatedSprite2D::add_animation(const std::string& name,
                                     const std::vector<int>& graph_ids,
                                     float fps, bool loop)
{
    Animation anim;
    anim.fps  = fps;
    anim.loop = loop;
    anim.frames.reserve(graph_ids.size());
    for (int gid : graph_ids)
        anim.frames.push_back(Frame{gid, 0.0f});
    m_animations[name] = std::move(anim);
}

void AnimatedSprite2D::add_animation_frames(const std::string& name,
                                            const std::vector<Frame>& frames,
                                            float default_fps, bool loop)
{
    Animation anim;
    anim.fps    = default_fps;
    anim.loop   = loop;
    anim.frames = frames;
    m_animations[name] = std::move(anim);
}

bool AnimatedSprite2D::has_animation(const std::string& name) const
{
    return m_animations.count(name) != 0;
}

void AnimatedSprite2D::remove_animation(const std::string& name)
{
    if (m_current == name) stop();
    m_animations.erase(name);
}

// ----------------------------------------------------------------------------
// Playback
// ----------------------------------------------------------------------------

void AnimatedSprite2D::play(const std::string& name, bool restart)
{
    if (m_current == name && m_playing && !restart) return;

    auto it = m_animations.find(name);
    if (it == m_animations.end())
    {
        TraceLog(LOG_WARNING, "AnimatedSprite2D::play: unknown animation '%s'", name.c_str());
        return;
    }

    m_current  = name;
    m_playing  = true;
    m_paused   = false;
    if (restart || m_frame >= (int)it->second.frames.size())
        m_frame = 0;
    m_elapsed  = 0.0f;
}

void AnimatedSprite2D::stop()
{
    m_playing = false;
    m_paused  = false;
    m_frame   = 0;
    m_elapsed = 0.0f;
}

void AnimatedSprite2D::pause()
{
    if (m_playing) m_paused = true;
}

void AnimatedSprite2D::resume()
{
    if (m_paused) m_paused = false;
}

void AnimatedSprite2D::set_frame(int frame)
{
    auto it = m_animations.find(m_current);
    if (it == m_animations.end()) return;
    const int count = (int)it->second.frames.size();
    m_frame   = (count > 0) ? (frame % count) : 0;
    m_elapsed = 0.0f;
}

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

float AnimatedSprite2D::frame_duration(const Animation& anim, int frame) const
{
    if (frame < 0 || frame >= (int)anim.frames.size()) return 0.0f;
    const float d = anim.frames[frame].duration;
    return (d > 0.0f) ? d : (1.0f / anim.fps);
}

// ----------------------------------------------------------------------------
// Update
// ----------------------------------------------------------------------------

void AnimatedSprite2D::_update(float dt)
{
    Node2D::_update(dt);

    if (!m_playing || m_paused || m_current.empty()) return;

    auto it = m_animations.find(m_current);
    if (it == m_animations.end()) return;

    const Animation& anim  = it->second;
    const int        count = (int)anim.frames.size();
    if (count == 0) return;

    m_elapsed += dt;

    while (m_elapsed >= frame_duration(anim, m_frame))
    {
        m_elapsed -= frame_duration(anim, m_frame);
        const int prev = m_frame;
        m_frame++;

        if (m_frame >= count)
        {
            if (anim.loop)
            {
                m_frame = 0;
            }
            else
            {
                m_frame   = count - 1;
                m_playing = false;
                if (on_animation_finished) on_animation_finished(m_current);
                break;
            }
        }

        if (m_frame != prev && on_frame_changed)
            on_frame_changed(m_frame);
    }
}

// ----------------------------------------------------------------------------
// Draw
// ----------------------------------------------------------------------------

void AnimatedSprite2D::_draw()
{
    Node2D::_draw();

    if (m_current.empty()) return;

    auto it = m_animations.find(m_current);
    if (it == m_animations.end()) return;

    const Animation& anim = it->second;
    if (m_frame < 0 || m_frame >= (int)anim.frames.size()) return;

    const int gid = anim.frames[m_frame].graph_id;
    if (gid < 0) return;

    Graph* g = GraphLib::Instance().getGraph(gid);
    if (!g) return;

    Texture2D* tex = GraphLib::Instance().getTexture(g->texture);
    if (!tex || tex->id == 0) return;

    const Matrix2D xf  = get_global_transform();
    const Vec2     pos = xf.TransformCoords(offset);

    // Scale factors from the transform
    const float sx = sqrtf(xf.a * xf.a + xf.b * xf.b);
    const float sy = sqrtf(xf.c * xf.c + xf.d * xf.d);

    Rectangle src = g->clip;
    if (flip_h) { src.x += src.width;  src.width  = -src.width;  }
    if (flip_v) { src.y += src.height; src.height = -src.height; }

    const float angle = atan2f(xf.b, xf.a) * (180.0f / 3.14159265f);
    const Vec2  pivot = {g->points.empty() ? src.width  * 0.5f : g->points[0].x,
                         g->points.empty() ? src.height * 0.5f : g->points[0].y};

    Rectangle dst{pos.x, pos.y,
                  fabsf(src.width)  * sx,
                  fabsf(src.height) * sy};

    DrawTexturePro(*tex, src, dst, {pivot.x * sx, pivot.y * sy}, angle, tint);
}
