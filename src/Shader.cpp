#include "Shader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

namespace {
GLuint compile(GLenum type, const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read shader: " + path.string());
    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string source = buffer.str();
    const char* text = source.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(static_cast<std::size_t>(length) + 1);
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compilation failed: " + path.string() + "\n" + log.data());
    }
    return shader;
}
}

Shader::Shader(const std::filesystem::path& vertex, const std::filesystem::path& fragment) {
    const GLuint vs = compile(GL_VERTEX_SHADER, vertex);
    GLuint fs = 0;
    try { fs = compile(GL_FRAGMENT_SHADER, fragment); }
    catch (...) { glDeleteShader(vs); throw; }
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint success = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(static_cast<std::size_t>(length) + 1);
        glGetProgramInfoLog(program_, length, nullptr, log.data());
        glDeleteProgram(program_);
        throw std::runtime_error(std::string("Shader link failed: ") + log.data());
    }
}

Shader::~Shader() { glDeleteProgram(program_); }
void Shader::use() const { glUseProgram(program_); }
void Shader::set(const char* name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(program_, name), 1, GL_FALSE, glm::value_ptr(value));
}
void Shader::set(const char* name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(program_, name), 1, glm::value_ptr(value));
}
void Shader::set(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(program_, name), value);
}
