#include "pch.hpp"
#include "ParticleEffectFactory.hpp"
#include "ParticleEmitter2D.hpp"

ParticleEmitter2D* ParticleEffectFactory::create_fire(const std::string& name, int graph)
{
    ParticleEmitter2D* fx = new ParticleEmitter2D(name);
    fx->graph = graph;
    fx->SetDirection(0.0f, -1.0f);
    fx->SetAngle(28.0f);
    fx->SetSelocityRange(20.0f, 48.0f);
    fx->SetFrequency(42.0f);
    fx->SetLife(0.9f);
    fx->SetSize(0.35f);
    fx->SetSizeStart(0.45f, 0.0f);
    fx->SetSizeEnd(1.2f, 0.9f);
    fx->SetAlphaStart(235.0f, 0.0f);
    fx->SetAlphaEnd(0.0f, 0.9f);
    fx->SetColorStart(255, 230, 110, 0.0f);
    fx->SetColorEnd(255, 80, 20, 0.9f);
    fx->gravity = Vec2(0.0f, -10.0f);
    fx->face_direction = false;
    fx->blend = BLEND_ADDITIVE;
    fx->SetStartZone(-8.0f, -4.0f, 8.0f, 4.0f);
    return fx;
}

ParticleEmitter2D* ParticleEffectFactory::create_smoke(const std::string& name, int graph)
{
    ParticleEmitter2D* fx = new ParticleEmitter2D(name);
    fx->graph = graph;
    fx->SetDirection(0.0f, -1.0f);
    fx->SetAngle(36.0f);
    fx->SetSelocityRange(10.0f, 24.0f);
    fx->SetFrequency(16.0f);
    fx->SetLife(1.8f);
    fx->SetSize(0.25f);
    fx->SetSizeStart(0.5f, 0.0f);
    fx->SetSizeEnd(1.6f, 1.8f);
    fx->SetAlphaStart(150.0f, 0.0f);
    fx->SetAlphaEnd(0.0f, 1.8f);
    fx->SetColorStart(180, 180, 180, 0.0f);
    fx->SetColorEnd(70, 70, 70, 1.8f);
    fx->gravity = Vec2(0.0f, -6.0f);
    fx->drag = 0.15f;
    fx->SetStartZone(-10.0f, -4.0f, 10.0f, 4.0f);
    return fx;
}

ParticleEmitter2D* ParticleEffectFactory::create_explosion(const std::string& name, int graph)
{
    ParticleEmitter2D* fx = new ParticleEmitter2D(name);
    fx->graph = graph;
    fx->SetOneShot();
    fx->SetDirection(1.0f, 0.0f);
    fx->SetAngle(360.0f);
    fx->SetSelocityRange(50.0f, 180.0f);
    fx->SetFrequency(220.0f);
    fx->SetLife(0.65f);
    fx->SetSize(0.45f);
    fx->SetSizeStart(0.35f, 0.0f);
    fx->SetSizeEnd(1.4f, 0.65f);
    fx->SetAlphaStart(255.0f, 0.0f);
    fx->SetAlphaEnd(0.0f, 0.65f);
    fx->SetColorStart(255, 245, 170, 0.0f);
    fx->SetColorEnd(255, 60, 10, 0.65f);
    fx->SetRotationRate(-360.0f, 360.0f);
    fx->blend = BLEND_ADDITIVE;
    fx->SetMaxParticles(48);
    return fx;
}
