#pragma once

#include <filesystem>
#include <glad/gl.h>

// Flattened static OBJ meshes share one VAO; each model keeps its assembly-space vertices.
class Model {
public:
    explicit Model(const std::filesystem::path& path);
    ~Model();
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    void draw() const;

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    GLsizei indexCount_ = 0;
};
