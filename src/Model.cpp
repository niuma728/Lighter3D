#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
struct Vertex { float position[3]; float normal[3]; };
}

Model::Model(const std::filesystem::path& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices);
    if (!scene || !scene->HasMeshes()) {
        throw std::runtime_error("Cannot load model " + path.string() + ": " + importer.GetErrorString());
    }
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh& mesh = *scene->mMeshes[m];
        if (!mesh.HasNormals()) throw std::runtime_error("Missing normals: " + path.string());
        const auto base = static_cast<unsigned int>(vertices.size());
        for (unsigned int i = 0; i < mesh.mNumVertices; ++i) {
            const auto& p = mesh.mVertices[i];
            const auto& n = mesh.mNormals[i];
            vertices.push_back({{p.x, p.y, p.z}, {n.x, n.y, n.z}});
        }
        for (unsigned int f = 0; f < mesh.mNumFaces; ++f) {
            const auto& face = mesh.mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (unsigned int i = 0; i < 3; ++i) indices.push_back(base + face.mIndices[i]);
        }
    }
    if (indices.empty()) throw std::runtime_error("Model has no triangles: " + path.string());
    indexCount_ = static_cast<GLsizei>(indices.size());
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glBindVertexArray(0);
    std::cout << path.filename().string() << ": " << indices.size() / 3 << " triangles\n";
}

Model::~Model() {
    glDeleteBuffers(1, &ebo_);
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

void Model::draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}
