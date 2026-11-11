#include "DemoController.h"
#include "Lighter.h"

#include <algorithm>

void DemoController::start(Lighter& lighter) {
    lighter.reset();
    step_ = 1;
    elapsed_ = 0.0f;
}

void DemoController::update(float dt, Lighter& lighter) {
    if (!active()) return;
    elapsed_ += std::max(dt, 0.0f);
    switch (step_) {
    case 1:
        if (elapsed_ >= 1.0f) { lighter.openLid(); next(); }
        break;
    case 2:
        if (lighter.state() == LighterState::Open) next();
        break;
    case 3:
        if (elapsed_ >= 1.0f) { lighter.ignite(); next(); }
        break;
    case 4:
        if (lighter.state() == LighterState::Burning) next();
        break;
    case 5:
        if (elapsed_ >= 3.0f) { lighter.extinguish(); next(); }
        break;
    case 6:
        if (lighter.state() == LighterState::Open) { lighter.ignite(); next(); }
        break;
    case 7:
        if (lighter.state() == LighterState::Burning) next();
        break;
    case 8:
        if (elapsed_ >= 2.0f) { lighter.closeLid(); next(); }
        break;
    case 9:
        if (lighter.state() == LighterState::Closed) stop();
        break;
    }
}
