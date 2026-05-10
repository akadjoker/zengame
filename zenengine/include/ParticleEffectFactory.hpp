#pragma once

#include "pch.hpp"

class ParticleEmitter2D;

class ParticleEffectFactory
{
public:
    static ParticleEmitter2D* create_fire(const std::string& name, int graph);
    static ParticleEmitter2D* create_smoke(const std::string& name, int graph);
    static ParticleEmitter2D* create_explosion(const std::string& name, int graph);
};
