#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

glm::vec3 Camera::position() const {
    const float yaw = glm::radians(yaw_);
    const float pitch = glm::radians(pitch_);
    return target_ + distance_ * glm::vec3(std::cos(pitch) * std::sin(yaw), std::sin(pitch),
                                 std::cos(pitch) * std::cos(yaw));
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position(), target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::orbit(float dx, float dy) {
    yaw_ = std::remainder(yaw_ + dx * 0.25f, 360.0f);
    pitch_ = std::clamp(pitch_ + dy * 0.25f, -80.0f, 80.0f);
}

void Camera::zoom(float offset) {
    distance_ = std::clamp(distance_ - offset * 0.5f, 3.5f, 12.0f);
}

void Camera::pan(float dx, float dy) {
    const glm::mat4 matrix = view();
    const glm::vec3 right(matrix[0][0], matrix[1][0], matrix[2][0]);
    const glm::vec3 up(matrix[0][1], matrix[1][1], matrix[2][1]);
    target_ += (-right * dx + up * dy) * (distance_ * 0.001f);
    target_ = glm::clamp(target_, glm::vec3(-3.0f), glm::vec3(3.0f));
}

void Camera::reset() { *this = Camera{}; }
