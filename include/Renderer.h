#pragma once

#include "Model.h"
#include "Shader.h"
#include "EffectsRenderer.h"
#include "Hud.h"

class Camera;
class Simulation;

class Renderer {
public:
    explicit Renderer(const std::filesystem::path& resources);
    void render(const Simulation& simulation, const Camera& camera, const std::string& notice,
                int width, int height, float fps = 0.0f);

private:
    Shader metal_;
    Model body_;
    Model lid_;
    Model hingeFixed_;
    Model hingeLid_;
    Model insert_;
    Model chimney_;
    Model wheel_;
    Model supports_;
    Model flint_;
    Model wick_;
    Model cam_;
    Model flintGuide_;
    EffectsRenderer effects_;
    Hud hud_;
};
