#include "Hud.h"
#include "Simulation.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <string_view>
#include <glm/gtc/matrix_transform.hpp>

namespace {
// A small 5x7 alphabet keeps the keyboard HUD independent of font libraries.
constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-./";
constexpr unsigned char glyphs[][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
    {7,2,2,2,2,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8}, {14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}, {0,4,4,0,4,4,0}, {0,0,0,31,0,0,0},
    {0,0,0,0,0,12,12}, {1,1,2,4,8,16,16}
};
}

Hud::Hud(const std::filesystem::path& root)
    : shader_(root / "shaders/hud.vert", root / "shaders/hud.frag") {
    vertices_.reserve(60000);
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.capacity() * sizeof(Vertex)), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    glBindVertexArray(0);
}

Hud::~Hud() {
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

void Hud::rectangle(float x, float y, float w, float h, const glm::vec4& color) {
    for (const auto& p : std::array<glm::vec2, 6>{{{x,y}, {x+w,y}, {x+w,y+h}, {x,y}, {x+w,y+h}, {x,y+h}}}) {
        vertices_.push_back({p, color});
    }
}

void Hud::text(float x, float y, const std::string& value, float scale, const glm::vec4& color) {
    for (unsigned char character : value) {
        const auto index = alphabet.find(static_cast<char>(std::toupper(character)));
        if (index != std::string_view::npos) {
            for (int row = 0; row < 7; ++row) {
                for (int column = 0; column < 5; ++column) {
                    if ((glyphs[index][row] & (1 << (4 - column))) != 0) {
                        rectangle(x + column * scale, y + row * scale, scale, scale, color);
                    }
                }
            }
        }
        x += 6.0f * scale;
    }
}

void Hud::draw(const Simulation& simulation, const std::string& notice, int width, int height, float fps) {
    vertices_.clear();
    const float scale = std::clamp(static_cast<float>(width) / 1100.0f, 0.5f, 1.5f);
    const float y = height - 104.0f * scale;
    const glm::vec4 white(0.78f, 0.83f, 0.9f, 1.0f), accent(0.96f, 0.70f, 0.30f, 1.0f);
    rectangle(16 * scale, y, width - 32 * scale, 92 * scale, {0.035f, 0.05f, 0.075f, 0.94f});
    text(30 * scale, y + 12 * scale, std::string("STATE: ") + simulation.lighter.stateName(), 1.5f * scale, accent);
    text(30 * scale, y + 32 * scale, simulation.demo.active() ? "DEMO: STEP " + std::to_string(simulation.demo.step()) : "DEMO: OFF", 1.5f * scale, white);
    text(30 * scale, y + 52 * scale, "FPS: " + std::to_string(static_cast<int>(fps)) + "  PARTICLES: " + std::to_string(simulation.particles.activeCount()), 1.5f * scale, white);
    text(320 * scale, y + 12 * scale, "O OPEN  C CLOSE  F IGNITE  E EXTINGUISH", 1.5f * scale, white);
    text(320 * scale, y + 32 * scale, "SPACE DEMO  R RESET  ESC EXIT", 1.5f * scale, white);
    text(320 * scale, y + 52 * scale, "LEFT DRAG ORBIT  RIGHT DRAG PAN  WHEEL ZOOM", 1.5f * scale, white);
    text(30 * scale, y + 73 * scale, notice.substr(0, 100), 1.2f * scale, accent);
    shader_.use();
    shader_.set("projection", glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f));
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(vertices_.size() * sizeof(Vertex)), vertices_.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
