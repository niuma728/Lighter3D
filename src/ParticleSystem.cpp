#include "ParticleSystem.h"
#include "Assembly.h"

#include <algorithm>

float ParticleSystem::random(float low, float high) {
    return std::uniform_real_distribution<float>(low, high)(random_);
}

void ParticleSystem::burst() {
    for (auto& particle : sparks_) {
        particle.position = assembly::sparkOrigin;
        particle.velocity = {random(-2.5f, -0.6f), random(1.2f, 3.2f), random(-0.8f, 0.8f)};
        particle.life = particle.maxLife = random(0.08f, 0.25f);
    }
}

void ParticleSystem::smoke() {
    for (auto& particle : smoke_) {
        particle.position = assembly::flameAnchor + glm::vec3(random(-0.03f, 0.03f), 0.05f, random(-0.03f, 0.03f));
        particle.velocity = {random(-0.10f, 0.10f), random(0.25f, 0.55f), random(-0.10f, 0.10f)};
        particle.life = particle.maxLife = random(0.5f, 1.1f);
    }
}

void ParticleSystem::update(float dt) {
    dt = std::max(dt, 0.0f);
    const glm::vec3 gravity(0.0f, -5.0f, 0.0f);
    for (auto& particle : sparks_) {
        if (particle.life <= 0.0f) continue;
        const float step = std::min(dt, particle.life);
        particle.position += particle.velocity * step + gravity * (0.5f * step * step);
        particle.velocity += gravity * step;
        particle.life = std::max(0.0f, particle.life - dt);
    }
    for (auto& particle : smoke_) {
        if (particle.life <= 0.0f) continue;
        particle.position += particle.velocity * std::min(dt, particle.life);
        particle.life = std::max(0.0f, particle.life - dt);
    }
}

void ParticleSystem::reset() {
    sparks_ = {};
    smoke_ = {};
    random_.seed(0xC0FFEE);
}

std::size_t ParticleSystem::activeCount() const {
    const auto alive = [](const Particle& particle) { return particle.life > 0.0f; };
    return static_cast<std::size_t>(std::count_if(sparks_.begin(), sparks_.end(), alive)
        + std::count_if(smoke_.begin(), smoke_.end(), alive));
}
