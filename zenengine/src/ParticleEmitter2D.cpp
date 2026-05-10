#include "pch.hpp"
#include "ParticleEmitter2D.hpp"
#include "render.hpp"

namespace
{
float randf01()
{
    return (float)GetRandomValue(0, 65535) / 65535.0f;
}

float randf_range(float min_value, float max_value)
{
    return min_value + randf01() * (max_value - min_value);
}

float clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}
}

ParticleEmitter2D::ParticleEmitter2D(const std::string& p_name)
    : Node2D(p_name)
{
    color_start = {255, 255, 255, 0.0f};
    color_end = {255, 255, 255, 0.0f};
    size_start = {1.0f, 0.0f};
    size_end = {1.0f, 0.0f};
    alpha_start = {255, 0.0f};
    alpha_end = {0, 0.0f};

    update_capacity();
}

void ParticleEmitter2D::_ready()
{
    if (autostart)
    {
        play();
    }
}

void ParticleEmitter2D::_update(float dt)
{
    if (!emitting && m_alive_count == 0 && one_shot &&
        max_particles_to_emit >= 0 && m_emitted_particles >= max_particles_to_emit)
    {
        finished = true;
    }

    if (emitting)
    {
        if (max_particles_to_emit < 0 || m_emitted_particles < max_particles_to_emit)
        {
            m_emission_accumulator += frequency * dt;
        }

        while (m_emission_accumulator >= 1.0f)
        {
            if (max_particles_to_emit >= 0 && m_emitted_particles >= max_particles_to_emit)
            {
                emitting = false;
                break;
            }

            emit_one();
            m_emission_accumulator -= 1.0f;
        }

        if (one_shot && max_particles_to_emit >= 0 && m_emitted_particles >= max_particles_to_emit)
        {
            emitting = false;
        }
    }

    m_alive_count = 0;
    m_first_dead = (int)m_particles.size();

    for (size_t i = 0; i < m_particles.size(); ++i)
    {
        Particle& p = m_particles[i];
        if (!p.alive)
        {
            if ((int)i < m_first_dead)
            {
                m_first_dead = (int)i;
            }
            continue;
        }

        update_particle(p, dt);

        if (!p.alive)
        {
            if ((int)i < m_first_dead)
            {
                m_first_dead = (int)i;
            }
            continue;
        }

        ++m_alive_count;
    }

    if (m_first_dead == (int)m_particles.size())
    {
        m_first_dead = 0;
    }

    finished = one_shot &&
               !emitting &&
               m_alive_count == 0 &&
               max_particles_to_emit >= 0 &&
               m_emitted_particles >= max_particles_to_emit;

    if (finished && auto_remove_on_finish)
    {
        queue_free();
    }
}

void ParticleEmitter2D::_draw()
{
    if (m_alive_count <= 0)
    {
        return;
    }

    Graph* g = GraphLib::Instance().getGraph(graph);
    if (!g)
    {
        return;
    }

    Texture2D* tex = GraphLib::Instance().getTexture(g->texture);
    if (!tex || tex->id <= 0)
    {
        return;
    }

    const Matrix2D emitter_global = get_global_transform();
    Vec2 graph_pivot(0.0f, 0.0f);
    if (!g->points.empty())
    {
        graph_pivot = g->points[0];
    }

    int raylib_blend = BLEND_ALPHA;
    if (blend == BLEND_ADDITIVE)
    {
        raylib_blend = BLEND_ADDITIVE;
    }
    else if (blend == BLEND_MULTIPLIED)
    {
        raylib_blend = BLEND_MULTIPLIED;
    }

    BeginBlendMode(raylib_blend);

    for (const Particle& p : m_particles)
    {
        if (!p.alive)
        {
            continue;
        }

        Color color = sample_color(p.time);
        color.a = sample_alpha(p.time);

        const float s = p.scale * size;
        RenderTransformFlipClipOffsetRotateScale(
            *tex,
            draw_offset - graph_pivot * s,
            (float)g->width,
            (float)g->height,
            g->clip,
            flip_x,
            flip_y,
            color,
            &emitter_global,
            p.pos,
            p.rotation,
            s,
            s,
            blend);
    }

    EndBlendMode();
}

void ParticleEmitter2D::play()
{
    finished = false;
    emitting = true;
}

void ParticleEmitter2D::stop()
{
    emitting = false;
}

void ParticleEmitter2D::restart()
{
    clear_particles();
    ResetCounter();
    finished = false;
    emitting = true;
}

void ParticleEmitter2D::clear_particles()
{
    for (Particle& p : m_particles)
    {
        p = Particle();
    }

    m_alive_count = 0;
    m_first_dead = 0;
    m_emission_accumulator = 0.0f;
}

void ParticleEmitter2D::burst(int count)
{
    if (count <= 0)
    {
        return;
    }

    finished = false;

    for (int i = 0; i < count; ++i)
    {
        if (max_particles_to_emit >= 0 && m_emitted_particles >= max_particles_to_emit)
        {
            emitting = false;
            break;
        }

        emit_one();
    }
}

bool ParticleEmitter2D::is_finished() const
{
    return finished;
}

bool ParticleEmitter2D::is_reach_end() const
{
    return !emitting &&
           m_alive_count == 0 &&
           max_particles_to_emit >= 0 &&
           m_emitted_particles >= max_particles_to_emit;
}

int ParticleEmitter2D::get_alive_count() const
{
    return m_alive_count;
}

int ParticleEmitter2D::get_max_particles() const
{
    return (int)m_particles.size();
}

void ParticleEmitter2D::SetColorEnd(unsigned char r, unsigned char g, unsigned char b, float time)
{
    color_end = {r, g, b, time};
}

void ParticleEmitter2D::SetColorStart(unsigned char r, unsigned char g, unsigned char b, float time)
{
    color_start = {r, g, b, time};
}

void ParticleEmitter2D::SetSizeEnd(float value, float time)
{
    size_end = {value, time};
}

void ParticleEmitter2D::SetSizeStart(float value, float time)
{
    size_start = {value, time};
}

void ParticleEmitter2D::SetAlphaEnd(float value, float time)
{
    alpha_end.value = (unsigned char)clamp(value, 0.0f, 255.0f);
    alpha_end.time = time;
}

void ParticleEmitter2D::SetAlphaStart(float value, float time)
{
    alpha_start.value = (unsigned char)clamp(value, 0.0f, 255.0f);
    alpha_start.time = time;
}

void ParticleEmitter2D::SetFrequency(float value)
{
    if (value < 0.0f)
    {
        return;
    }

    frequency = value;
    update_capacity();
}

void ParticleEmitter2D::SetStartZone(float x1, float y1, float x2, float y2)
{
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    m_spawn_x1 = x1;
    m_spawn_y1 = y1;
    m_spawn_x2 = x2;
    m_spawn_y2 = y2;
}

void ParticleEmitter2D::SetDirection(float vx, float vy)
{
    direction.x = vx;
    direction.y = vy;
}

void ParticleEmitter2D::SetSelocityRange(float v1, float v2)
{
    if (v1 < 0.0f || v2 < 0.0f)
    {
        return;
    }

    if (v2 < v1) std::swap(v1, v2);

    speed_min = v1;
    speed_max = v2;
}

void ParticleEmitter2D::SetAngle(float angle_deg)
{
    if (angle_deg < 0.0f)
    {
        return;
    }

    spread = angle_deg;
    m_spread_rad = spread * DEG_TO_RAD;
}

void ParticleEmitter2D::SetAngleRad(float angle_rad)
{
    if (angle_rad < 0.0f)
    {
        return;
    }

    m_spread_rad = angle_rad;
    spread = angle_rad * RAD_TO_DEG;
}

void ParticleEmitter2D::ResetCounter()
{
    m_emitted_particles = 0;
}

void ParticleEmitter2D::SetSize(float value)
{
    if (value < 0.0f)
    {
        return;
    }

    size = value;
}

void ParticleEmitter2D::SetMaxParticles(int max)
{
    if (max < -1 || max == 0)
    {
        return;
    }

    max_particles_to_emit = max;
}

void ParticleEmitter2D::SetRotationRate(float a1_deg, float a2_deg)
{
    if (a2_deg < a1_deg) std::swap(a1_deg, a2_deg);
    rotation_rate_min = a1_deg;
    rotation_rate_max = a2_deg;
}

void ParticleEmitter2D::SetRotationRateRad(float a1_rad, float a2_rad)
{
    SetRotationRate(a1_rad * RAD_TO_DEG, a2_rad * RAD_TO_DEG);
}

void ParticleEmitter2D::SetFaceDirection(bool flag)
{
    face_direction = flag;
}

void ParticleEmitter2D::Offset(float x, float y)
{
    for (Particle& p : m_particles)
    {
        if (!p.alive)
        {
            continue;
        }

        p.pos.x += x;
        p.pos.y += y;
    }
}

void ParticleEmitter2D::SetEmitterPosition(float x, float y)
{
    emission_offset.x = x;
    emission_offset.y = y;
}

void ParticleEmitter2D::SetLife(float time)
{
    if (time <= 0.0f)
    {
        return;
    }

    life = time;
    update_capacity();
}

void ParticleEmitter2D::SetOneShot()
{
    one_shot = true;
    auto_remove_on_finish = true;
    if (max_particles_to_emit < 0)
    {
        max_particles_to_emit = get_max_particles();
    }
}

void ParticleEmitter2D::update_capacity()
{
    int target = (int)std::ceil(frequency * life) + 2;
    if (target < 1)
    {
        target = 1;
    }

    if ((int)m_particles.size() < target)
    {
        m_particles.resize((size_t)target);
    }

    if (one_shot && max_particles_to_emit < 0)
    {
        max_particles_to_emit = target;
    }
}

void ParticleEmitter2D::emit_one()
{
    int index = find_dead_particle();
    if (index < 0)
    {
        return;
    }

    emit_at(index);
}

void ParticleEmitter2D::emit_at(int index)
{
    Particle& p = m_particles[(size_t)index];

    float spawn_x = m_spawn_x1;
    float spawn_y = m_spawn_y1;
    if (m_spawn_x2 > m_spawn_x1)
    {
        spawn_x = randf_range(m_spawn_x1, m_spawn_x2);
    }
    if (m_spawn_y2 > m_spawn_y1)
    {
        spawn_y = randf_range(m_spawn_y1, m_spawn_y2);
    }

    p.pos = emission_offset + Vec2(spawn_x, spawn_y);
    p.time = 0.0f;
    p.scale = sample_scale(0.0f);
    p.alive = true;

    Vec2 dir = direction;
    float dir_len = dir.magnitude();
    if (dir_len < 1e-6f)
    {
        dir = Vec2(0.0f, -1.0f);
    }
    else
    {
        dir /= dir_len;
    }

    if (m_spread_rad > 0.0f)
    {
        float angle = m_spread_rad * (randf01() - 0.5f);
        dir = dir.rotate(angle);
    }

    float velocity = randf_range(speed_min, speed_max);
    p.vel = dir * velocity;

    p.rotation = 0.0f;
    p.rotation_delta = randf_range(rotation_rate_min, rotation_rate_max);

    if (face_direction)
    {
        p.rotation = std::atan2(p.vel.y, p.vel.x) * RAD_TO_DEG + face_dir_angle_offset;
    }

    ++m_emitted_particles;
    m_first_dead = index + 1;
    if (m_first_dead >= (int)m_particles.size())
    {
        m_first_dead = 0;
    }
}

int ParticleEmitter2D::find_dead_particle()
{
    if (m_particles.empty())
    {
        return -1;
    }

    for (int i = m_first_dead; i < (int)m_particles.size(); ++i)
    {
        if (!m_particles[(size_t)i].alive)
        {
            return i;
        }
    }

    for (int i = 0; i < m_first_dead; ++i)
    {
        if (!m_particles[(size_t)i].alive)
        {
            return i;
        }
    }

    return -1;
}

void ParticleEmitter2D::update_particle(Particle& p, float dt)
{
    p.time += dt;
    if (p.time > life)
    {
        p.alive = false;
        return;
    }

    p.vel += gravity * dt;

    if (drag > 0.0f)
    {
        float drag_factor = 1.0f - drag * dt;
        if (drag_factor < 0.0f)
        {
            drag_factor = 0.0f;
        }
        p.vel *= drag_factor;
    }

    p.pos += p.vel * dt;

    if (face_direction)
    {
        p.rotation = std::atan2(p.vel.y, p.vel.x) * RAD_TO_DEG + face_dir_angle_offset;
    }
    else
    {
        p.rotation += p.rotation_delta * dt;
    }

    p.scale = sample_scale(p.time);
}

Color ParticleEmitter2D::sample_color(float time) const
{
    float total = color_end.time - color_start.time;
    if (total <= 0.0f)
    {
        return Color{color_end.r, color_end.g, color_end.b, 255};
    }

    float t = clamp01((time - color_start.time) / total);
    return Color{
        (unsigned char)((1.0f - t) * color_start.r + t * color_end.r),
        (unsigned char)((1.0f - t) * color_start.g + t * color_end.g),
        (unsigned char)((1.0f - t) * color_start.b + t * color_end.b),
        255
    };
}

unsigned char ParticleEmitter2D::sample_alpha(float time) const
{
    float total = alpha_end.time - alpha_start.time;
    if (total <= 0.0f)
    {
        return alpha_end.value;
    }

    float t = clamp01((time - alpha_start.time) / total);
    return (unsigned char)((1.0f - t) * alpha_start.value + t * alpha_end.value);
}

float ParticleEmitter2D::sample_scale(float time) const
{
    float total = size_end.time - size_start.time;
    if (total <= 0.0f)
    {
        return size_end.value;
    }

    float t = clamp01((time - size_start.time) / total);
    return ((1.0f - t) * size_start.value + t * size_end.value);
}
