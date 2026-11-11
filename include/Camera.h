#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 position() const;
    glm::mat4 view() const;
    void orbit(float dx, float dy);
    void zoom(float offset);
    void pan(float dx, float dy);
    void reset();

private:
    glm::vec3 target_{-0.25f, 0.2f, 0.0f};
    float yaw_ = 25.0f;
    float pitch_ = 15.0f;
    float distance_ = 7.0f;
};
