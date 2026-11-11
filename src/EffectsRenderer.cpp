#include "EffectsRenderer.h"
#include "Assembly.h"
#include "Camera.h"
#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace {
// Match flame.vert so triangle order also follows the animated surface.
glm::vec3 flamePosition(const glm::vec4& vertex, float time, float scale) {
    const float h = vertex.y;
    const float height = vertex.w > 0.5f ? 0.68f : 1.0f;
    glm::vec3 p(vertex.x, h * 0.82f * height, vertex.z);
    p.x += 0.06f * h * h * height * std::sin(time * 7.0f + 5.0f * h);
    p.z += 0.035f * h * h * height * std::sin(time * 9.0f + 6.0f * h + 1.0f);
    p.y *= 1.0f + 0.035f * std::sin(time * 11.0f) + 0.02f * std::sin(time * 17.0f);
    return assembly::flameAnchor + p * scale;
}
}

EffectsRenderer::EffectsRenderer(const std::filesystem::path& root)
    : flame_(root / "shaders/flame.vert", root / "shaders/flame.frag"),
      particle_(root / "shaders/particle.vert", root / "shaders/particle.frag") {
    constexpr int rings = 24, sides = 24;
    for (int layer = 0; layer < 2; ++layer) {
        const auto base = static_cast<unsigned int>(vertices_.size());
        for (int ring = 0; ring <= rings; ++ring) {
            const float h = static_cast<float>(ring) / rings;
            const float profile = 0.025f * (1.0f - h) + 0.16f * std::pow(std::max(0.0f, std::sin(3.14159265f * h)), 0.8f) * (1.0f - 0.65f * h);
            const float radius = profile * (layer == 0 ? 1.0f : 0.56f);
            for (int side = 0; side < sides; ++side) {
                const float angle = 6.2831853f * side / sides;
                vertices_.emplace_back(radius * std::cos(angle), h, radius * std::sin(angle), static_cast<float>(layer));
            }
        }
        for (int ring = 0; ring < rings; ++ring) {
            for (int side = 0; side < sides; ++side) {
                const auto a = base + ring * sides + side;
                const auto b = base + ring * sides + (side + 1) % sides;
                faces_.push_back({a, b, a + sides, 0.0f});
                faces_.push_back({b, b + sides, a + sides, 0.0f});
            }
        }
    }
    indices_.resize(faces_.size() * 3);
    for (const auto& vertex : vertices_) {
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z)) {
            throw std::runtime_error("Flame mesh contains a non-finite vertex");
        }
    }
    positions_.resize(vertices_.size());
    points_.reserve(ParticleSystem::sparkCapacity + ParticleSystem::smokeCapacity);
    glGenVertexArrays(1, &flameVao_);
    glGenBuffers(1, &flameVbo_);
    glGenBuffers(1, &flameEbo_);
    glBindVertexArray(flameVao_);
    glBindBuffer(GL_ARRAY_BUFFER, flameVbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(glm::vec4)), vertices_.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, flameEbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices_.size() * sizeof(unsigned int)), nullptr, GL_DYNAMIC_DRAW);
    glGenVertexArrays(1, &pointVao_);
    glGenBuffers(1, &pointVbo_);
    glBindVertexArray(pointVao_);
    glBindBuffer(GL_ARRAY_BUFFER, pointVbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(points_.capacity() * sizeof(Point)), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Point), reinterpret_cast<void*>(offsetof(Point, life)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Point), reinterpret_cast<void*>(offsetof(Point, smoke)));
    glBindVertexArray(0);
}

EffectsRenderer::~EffectsRenderer() {
    glDeleteBuffers(1, &pointVbo_);
    glDeleteVertexArrays(1, &pointVao_);
    glDeleteBuffers(1, &flameEbo_);
    glDeleteBuffers(1, &flameVbo_);
    glDeleteVertexArrays(1, &flameVao_);
}

void EffectsRenderer::draw(const Camera& camera, const glm::mat4& projection,
                          const ParticleSystem& particles, float time, float flameScale, int height) {
    const glm::mat4 view = camera.view();
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (flameScale > 0.0f) {
        for (std::size_t i = 0; i < vertices_.size(); ++i) positions_[i] = flamePosition(vertices_[i], time, flameScale);
        for (auto& face : faces_) {
            const glm::vec3 center = (positions_[face.a] + positions_[face.b] + positions_[face.c]) / 3.0f;
            face.depth = (view * glm::vec4(center, 1.0f)).z;
        }
        std::sort(faces_.begin(), faces_.end(), [](const Face& a, const Face& b) { return a.depth < b.depth; });
        for (std::size_t i = 0; i < faces_.size(); ++i) {
            indices_[3 * i] = faces_[i].a;
            indices_[3 * i + 1] = faces_[i].b;
            indices_[3 * i + 2] = faces_[i].c;
        }
        flame_.use();
        flame_.set("view", view);
        flame_.set("projection", projection);
        flame_.set("anchor", assembly::flameAnchor);
        flame_.set("time", time);
        flame_.set("scale", flameScale);
        glBindVertexArray(flameVao_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, flameEbo_);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(indices_.size() * sizeof(unsigned int)), indices_.data());
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
    }
    particle_.use();
    particle_.set("view", view);
    particle_.set("projection", projection);
    particle_.set("viewportHeight", static_cast<float>(height));
    glEnable(GL_PROGRAM_POINT_SIZE);
    glBindVertexArray(pointVao_);
    glBindBuffer(GL_ARRAY_BUFFER, pointVbo_);
    points_.clear();
    for (const auto& p : particles.smokeParticles()) {
        if (p.life > 0.0f) points_.push_back({p.position, p.life / p.maxLife, 1.0f});
    }
    std::sort(points_.begin(), points_.end(), [&view](const Point& a, const Point& b) {
        return (view * glm::vec4(a.position, 1.0f)).z < (view * glm::vec4(b.position, 1.0f)).z;
    });
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(points_.size() * sizeof(Point)), points_.data());
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points_.size()));
    points_.clear();
    for (const auto& p : particles.sparks()) {
        if (p.life > 0.0f) points_.push_back({p.position, p.life / p.maxLife, 0.0f});
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(points_.size() * sizeof(Point)), points_.data());
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points_.size()));
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}
