#pragma once
#include "Shader.h"
#include <string>
#include <vector>

class Simulation;
class Hud {
public:
    explicit Hud(const std::filesystem::path& root);
    ~Hud();
    Hud(const Hud&) = delete;
    Hud& operator=(const Hud&) = delete;
    void draw(const Simulation& simulation, const std::string& notice, int width, int height, float fps);

private:
    struct Vertex { glm::vec2 position; glm::vec4 color; };
    void rectangle(float x, float y, float w, float h, const glm::vec4& color);
    void text(float x, float y, const std::string& value, float scale, const glm::vec4& color);
    Shader shader_;
    GLuint vao_ = 0, vbo_ = 0;
    std::vector<Vertex> vertices_;
};
