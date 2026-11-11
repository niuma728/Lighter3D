#pragma once

#include "Animation.h"

enum class LighterState { Closed, Opening, Open, Igniting, Burning, Extinguishing, Closing };

class Lighter {
public:
    static constexpr float openAngle = 115.0f;
    bool openLid();
    bool closeLid();
    bool ignite();
    bool extinguish();
    void update(float dt);
    void reset();
    LighterState state() const { return state_; }
    float lidAngle() const { return lidAngle_; }
    float wheelAngle() const { return wheelAngle_; }
    float flameScale() const { return flameScale_; }
    bool takeSparkEvent();
    bool takeSmokeEvent();
    const char* stateName() const;

private:
    void moveLid(float target, LighterState state);
    void beginExtinguish(bool closeAfter);
    LighterState state_ = LighterState::Closed;
    float lidAngle_ = 0.0f;
    float wheelAngle_ = 0.0f;
    float flameScale_ = 0.0f;
    float ignitionTime_ = 0.0f;
    bool closeAfterExtinguish_ = false;
    bool sparkEvent_ = false;
    bool smokeEvent_ = false;
    Animation lidAnimation_;
    Animation extinguishAnimation_;
};
