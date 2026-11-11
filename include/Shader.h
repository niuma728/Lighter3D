#pragma once

#include <filesystem>
#include <glad/gl.h>
#include <glm/glm.hpp>

class Shader {
public:
    Shader(const std::filesystem::path& vertex, const std::filesystem::path& fragment);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    void use() const;
    void set(const char* name, const glm::mat4& value) const;
    void set(const char* name, const glm::vec3& value) const;
    void set(const char* name, float value) const;

private:
    GLuint program_ = 0;
};
