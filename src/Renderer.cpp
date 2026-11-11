#include "Renderer.h"
#include "Assembly.h"
#include "Camera.h"
#include "Simulation.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Renderer::Renderer(const std::filesystem::path& root)
    : metal_(root / "shaders/metal.vert", root / "shaders/metal.frag"),
      body_(root / "assets/models/body.obj"), lid_(root / "assets/models/lid.obj"),
      hingeFixed_(root / "assets/models/hinge_fixed.obj"), hingeLid_(root / "assets/models/hinge_lid.obj"),
      insert_(root / "assets/models/insert.obj"), chimney_(root / "assets/models/chimney.obj"),
      wheel_(root / "assets/models/wheel.obj"), supports_(root / "assets/models/supports.obj"),
      flint_(root / "assets/models/flint.obj"), wick_(root / "assets/models/wick.obj"),
      cam_(root / "assets/models/cam.obj"), flintGuide_(root / "assets/models/flint_guide.obj"),
      effects_(root), hud_(root) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
}

void Renderer::render(const Simulation& simulation, const Camera& camera, const std::string& notice,
                      int width, int height, float fps) {
    if (width <= 0 || height <= 0) return;
    glViewport(0, 0, width, height);
    glClearColor(0.035f, 0.045f, 0.065f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    metal_.use();
    const auto& lighter = simulation.lighter;
    const glm::mat4 projection = glm::perspective(glm::radians(42.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
    metal_.set("view", camera.view());
    metal_.set("projection", projection);
    metal_.set("eye", camera.position());
    metal_.set("flamePosition", assembly::flameAnchor);
    metal_.set("flamePower", lighter.flameScale() * (0.96f + 0.04f * std::sin(simulation.time() * 11.0f)));
    metal_.set("baseColor", glm::vec3(0.64f, 0.68f, 0.72f));
    metal_.set("shininess", 96.0f);
    metal_.set("specularStrength", 0.85f);
    metal_.set("model", glm::mat4(1.0f));
    body_.draw();
    const glm::mat4 lidTransform = glm::translate(glm::mat4(1.0f), assembly::hingePivot)
        * glm::rotate(glm::mat4(1.0f), glm::radians(lighter.lidAngle()), assembly::hingeAxis)
        * glm::translate(glm::mat4(1.0f), -assembly::hingePivot);
    metal_.set("model", lidTransform);
    lid_.draw();
    metal_.set("baseColor", glm::vec3(0.36f, 0.39f, 0.43f));
    hingeLid_.draw();
    metal_.set("model", glm::mat4(1.0f));
    hingeFixed_.draw();
    metal_.set("baseColor", glm::vec3(0.47f, 0.50f, 0.52f));
    metal_.set("shininess", 40.0f);
    metal_.set("specularStrength", 0.55f);
    insert_.draw();
    chimney_.draw();
    supports_.draw();
    metal_.set("baseColor", glm::vec3(0.055f, 0.06f, 0.065f));
    metal_.set("shininess", 24.0f);
    metal_.set("specularStrength", 0.18f);
    flint_.draw();
    const float camAngle = assembly::camClosedAngle * (1.0f - lighter.lidAngle() / Lighter::openAngle);
    const glm::mat4 camTransform = glm::translate(glm::mat4(1.0f), assembly::camPivot)
        * glm::rotate(glm::mat4(1.0f), glm::radians(camAngle), assembly::hingeAxis)
        * glm::translate(glm::mat4(1.0f), -assembly::camPivot);
    metal_.set("model", camTransform);
    cam_.draw();
    const glm::mat4 wheelTransform = glm::translate(glm::mat4(1.0f), assembly::wheelPivot)
        * glm::rotate(glm::mat4(1.0f), glm::radians(lighter.wheelAngle()), assembly::wheelAxis)
        * glm::translate(glm::mat4(1.0f), -assembly::wheelPivot);
    metal_.set("model", wheelTransform);
    wheel_.draw();
    metal_.set("model", glm::mat4(1.0f));
    metal_.set("baseColor", glm::vec3(0.60f, 0.43f, 0.12f));
    metal_.set("shininess", 64.0f);
    metal_.set("specularStrength", 0.65f);
    flintGuide_.draw();
    metal_.set("baseColor", glm::vec3(0.32f, 0.22f, 0.11f));
    metal_.set("shininess", 4.0f);
    metal_.set("specularStrength", 0.06f);
    wick_.draw();
    effects_.draw(camera, projection, simulation.particles, simulation.time(), lighter.flameScale(), height);
    hud_.draw(simulation, notice, width, height, fps);
}
