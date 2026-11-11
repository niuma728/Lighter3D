#include "Simulation.h"

#include <algorithm>
#include <cmath>

void Simulation::update(float dt) {
    float remaining = std::max(dt, 0.0f);
    do {
        const float step = std::min(remaining, 1.0f / 120.0f);
        demo.update(step, lighter);
        particles.update(step);
        lighter.update(step);
        if (lighter.takeSparkEvent()) particles.burst();
        if (lighter.takeSmokeEvent()) particles.smoke();
        const auto state = lighter.state();
        if (state == LighterState::Closed || state == LighterState::Closing || state == LighterState::Open) {
            particles.clearSparks();
        }
        time_ = std::fmod(time_ + step, 3600.0f);
        remaining -= step;
    } while (remaining > 0.0f);
}

void Simulation::reset() {
    lighter.reset();
    demo.stop();
    particles.reset();
    time_ = 0.0f;
}

void Simulation::toggleDemo() {
    if (demo.active()) demo.stop();
    else {
        demo.start(lighter);
        particles.reset();
        time_ = 0.0f;
    }
}
