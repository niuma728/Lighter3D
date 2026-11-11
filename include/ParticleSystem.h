#pragma once

#include <array>
#include <random>
#include <glm/glm.hpp>

struct Particle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float life = 0.0f;
    float maxLife = 1.0f;
};

class ParticleSystem {
public:
    static constexpr std::size_t sparkCapacity = 64;
    static constexpr std::size_t smokeCapacity = 24;
    void burst();
    void smoke();
    void update(float dt);
    void clearSparks() { sparks_ = {}; }
    void reset();
    const auto& sparks() const { return sparks_; }
    const auto& smokeParticles() const { return smoke_; }
    std::size_t activeCount() const;

private:
    float random(float low, float high);
    std::array<Particle, sparkCapacity> sparks_{};
    std::array<Particle, smokeCapacity> smoke_{};
    std::mt19937 random_{0xC0FFEE};
};
