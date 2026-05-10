#pragma once

#include "pch.hpp"
#include "Node2D.hpp"
#include "Assets.hpp"
#include "math.hpp"

class ParticleEmitter2D : public Node2D
{
public:
    struct ColorKey
    {
        unsigned char r = 255;
        unsigned char g = 255;
        unsigned char b = 255;
        float time = 0.0f;
    };

    struct SizeKey
    {
        float value = 1.0f;
        float time = 0.0f;
    };

    struct AlphaKey
    {
        unsigned char value = 255;
        float time = 0.0f;
    };

    explicit ParticleEmitter2D(const std::string& p_name = "ParticleEmitter2D");

    int   graph = 0;
    bool  flip_x = false;
    bool  flip_y = false;
    int   blend = 0;
    bool  one_shot = false;
    bool  face_direction = false;
    bool  emitting = true;
    bool  finished = false;
    bool  autostart = true;
    bool  auto_remove_on_finish = false;

    Vec2  emission_offset = Vec2(0.0f, 0.0f);
    Vec2  direction = Vec2(0.0f, -1.0f);
    Vec2  gravity = Vec2(0.0f, 0.0f);
    Vec2  draw_offset = Vec2(0.0f, 0.0f);

    float spread = 10.0f;
    float speed_min = 1.0f;
    float speed_max = 1.0f;
    float size = 1.0f;
    float life = 3.0f;
    float frequency = 10.0f;
    float drag = 0.0f;
    float rotation_rate_min = 0.0f;
    float rotation_rate_max = 0.0f;
    float face_dir_angle_offset = 90.0f;

    int   max_particles_to_emit = -1;

    ColorKey color_start;
    ColorKey color_end;
    SizeKey  size_start;
    SizeKey  size_end;
    AlphaKey alpha_start;
    AlphaKey alpha_end;

    void _ready() override;
    void _update(float dt) override;
    void _draw() override;

    void play();
    void stop();
    void restart();
    void clear_particles();
    void burst(int count);

    bool is_finished() const;
    bool is_reach_end() const;
    int  get_alive_count() const;
    int  get_max_particles() const;

    void SetColorEnd(unsigned char r, unsigned char g, unsigned char b, float time);
    void SetColorStart(unsigned char r, unsigned char g, unsigned char b, float time);
    void SetSizeEnd(float value, float time);
    void SetSizeStart(float value, float time);
    void SetAlphaEnd(float value, float time);
    void SetAlphaStart(float value, float time);
    void SetFrequency(float value);
    void SetStartZone(float x1, float y1, float x2, float y2);
    void SetDirection(float vx, float vy);
    void SetSelocityRange(float v1, float v2);
    void SetAngle(float angle_deg);
    void SetAngleRad(float angle_rad);
    void ResetCounter();
    void SetSize(float value);
    void SetMaxParticles(int max);
    void SetRotationRate(float a1_deg, float a2_deg);
    void SetRotationRateRad(float a1_deg, float a2_deg);
    void SetFaceDirection(bool flag);
    void Offset(float x, float y);
    void SetEmitterPosition(float x, float y);
    void SetLife(float time);
    void SetOneShot();

private:
    struct Particle
    {
        Vec2 pos = Vec2(0.0f, 0.0f);
        Vec2 vel = Vec2(0.0f, 0.0f);
        float rotation = 0.0f;
        float rotation_delta = 0.0f;
        float scale = 1.0f;
        float time = 0.0f;
        bool  alive = false;
    };

    std::vector<Particle> m_particles;
    float m_spawn_x1 = 0.0f;
    float m_spawn_y1 = 0.0f;
    float m_spawn_x2 = 0.0f;
    float m_spawn_y2 = 0.0f;
    float m_emission_accumulator = 0.0f;
    float m_spread_rad = 10.0f * DEG_TO_RAD;
    int   m_emitted_particles = 0;
    int   m_alive_count = 0;
    int   m_first_dead = 0;

    void update_capacity();
    void emit_one();
    void emit_at(int index);
    int  find_dead_particle();
    void update_particle(Particle& p, float dt);
    Color sample_color(float time) const;
    unsigned char sample_alpha(float time) const;
    float sample_scale(float time) const;
};
