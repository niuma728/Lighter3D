#pragma once

#include <glm/glm.hpp>

// Same assembly coordinates as assets/source/create_lighter.py; OBJ bind transforms are identity.
namespace assembly {
inline const glm::vec3 hingePivot{-0.94f, 0.565f, 0.0f};
inline const glm::vec3 hingeAxis{0.0f, 0.0f, 1.0f};
inline const glm::vec3 wheelPivot{0.50f, 1.035f, 0.0f};
inline const glm::vec3 wheelAxis{0.0f, 0.0f, 1.0f};
inline const glm::vec3 sparkOrigin{0.50f, 0.81f, 0.14f};
inline const glm::vec3 flameAnchor{-0.08f, 1.235f, 0.0f};
inline const glm::vec3 camPivot{-0.60f, 0.86f, 0.0f};
inline constexpr float camClosedAngle = -65.0f;
}
