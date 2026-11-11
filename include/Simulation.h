#pragma once

#include "Lighter.h"
#include "DemoController.h"
#include "ParticleSystem.h"

class Simulation {
public:
    Lighter lighter;
    DemoController demo;
    ParticleSystem particles;
    void update(float dt);
    void reset();
    void toggleDemo();
    float time() const { return time_; }

private:
    float time_ = 0.0f;
};
