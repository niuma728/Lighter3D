#pragma once

#include "Shader.h"
#include <vector>

class Camera;
class ParticleSystem;

class EffectsRenderer {
public:
    explicit EffectsRenderer(const std::filesystem::path& root);
    ~EffectsRenderer();
    EffectsRenderer(const EffectsRenderer&) = delete;
    EffectsRenderer& operator=(const EffectsRenderer&) = delete;
    void draw(const Camera& camera, const glm::mat4& projection,
              const ParticleSystem& particles, float time, float flameScale, int height);

private:
    struct Face { unsigned int a, b, c; float depth; };
    struct Point { glm::vec3 position; float life; float smoke; };
    Shader flame_;
    Shader particle_;
    GLuint flameVao_ = 0, flameVbo_ = 0, flameEbo_ = 0;
    GLuint pointVao_ = 0, pointVbo_ = 0;
    std::vector<glm::vec4> vertices_;
    std::vector<Face> faces_;
    std::vector<unsigned int> indices_;
    std::vector<glm::vec3> positions_;
    std::vector<Point> points_;
};
