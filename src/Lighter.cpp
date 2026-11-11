#include "Lighter.h"

#include <cmath>
#include <algorithm>
#include <utility>

void Lighter::moveLid(float target, LighterState state) {
    lidAnimation_.start(lidAngle_, target, 0.5f * std::abs(target - lidAngle_) / openAngle);
    state_ = state;
}

bool Lighter::openLid() {
    if (state_ == LighterState::Extinguishing) {
        closeAfterExtinguish_ = false;
        return true;
    }
    if (state_ != LighterState::Closed && state_ != LighterState::Closing) return false;
    moveLid(openAngle, LighterState::Opening);
    return true;
}

bool Lighter::closeLid() {
    if (state_ == LighterState::Extinguishing) {
        closeAfterExtinguish_ = true;
        return true;
    }
    if (state_ == LighterState::Igniting || state_ == LighterState::Burning) {
        beginExtinguish(true);
        return true;
    }
    if (state_ != LighterState::Open && state_ != LighterState::Opening) return false;
    moveLid(0.0f, LighterState::Closing);
    return true;
}

bool Lighter::ignite() {
    if (state_ != LighterState::Open) return false;
    state_ = LighterState::Igniting;
    ignitionTime_ = 0.0f;
    sparkEvent_ = false;
    return true;
}

bool Lighter::extinguish() {
    if (state_ != LighterState::Igniting && state_ != LighterState::Burning) return false;
    beginExtinguish(false);
    return true;
}

void Lighter::beginExtinguish(bool closeAfter) {
    closeAfterExtinguish_ = closeAfter;
    sparkEvent_ = false;
    smokeEvent_ = flameScale_ > 0.0f;
    if (flameScale_ == 0.0f) {
        if (closeAfter) moveLid(0.0f, LighterState::Closing);
        else state_ = LighterState::Open;
        return;
    }
    extinguishAnimation_.start(flameScale_, 0.0f, 0.3f);
    state_ = LighterState::Extinguishing;
}

void Lighter::update(float dt) {
    float remaining = std::max(dt, 0.0f);
    // Consume leftover time across state boundaries, including extinguish -> close.
    for (;;) {
        if (state_ == LighterState::Opening || state_ == LighterState::Closing) {
            const float step = std::min(remaining, lidAnimation_.remaining());
            lidAngle_ = lidAnimation_.update(step);
            remaining -= step;
            if (!lidAnimation_.finished()) return;
            state_ = state_ == LighterState::Opening ? LighterState::Open : LighterState::Closed;
        } else if (state_ == LighterState::Igniting) {
            const float previous = ignitionTime_;
            const float step = std::min(remaining, 0.4f - previous);
            ignitionTime_ = std::min(previous + step, 0.4f);
            wheelAngle_ = std::fmod(wheelAngle_ + 3000.0f *
                (std::min(ignitionTime_, 0.18f) - std::min(previous, 0.18f)), 360.0f);
            if (previous < 0.08f && ignitionTime_ >= 0.08f) sparkEvent_ = true;
            const float u = std::clamp((ignitionTime_ - 0.15f) / 0.25f, 0.0f, 1.0f);
            flameScale_ = u * u * (3.0f - 2.0f * u);
            remaining -= step;
            if (ignitionTime_ < 0.4f) return;
            flameScale_ = 1.0f;
            state_ = LighterState::Burning;
        } else if (state_ == LighterState::Extinguishing) {
            const float step = std::min(remaining, extinguishAnimation_.remaining());
            flameScale_ = extinguishAnimation_.update(step);
            remaining -= step;
            if (!extinguishAnimation_.finished()) return;
            flameScale_ = 0.0f;
            if (closeAfterExtinguish_) moveLid(0.0f, LighterState::Closing);
            else state_ = LighterState::Open;
            closeAfterExtinguish_ = false;
        } else {
            return;
        }
        if (remaining <= 0.0f) return;
    }
}

void Lighter::reset() {
    *this = Lighter{};
}

bool Lighter::takeSparkEvent() { return std::exchange(sparkEvent_, false); }
bool Lighter::takeSmokeEvent() { return std::exchange(smokeEvent_, false); }

const char* Lighter::stateName() const {
    switch (state_) {
    case LighterState::Closed: return "Closed";
    case LighterState::Opening: return "Opening";
    case LighterState::Open: return "Open";
    case LighterState::Igniting: return "Igniting";
    case LighterState::Burning: return "Burning";
    case LighterState::Extinguishing: return "Extinguishing";
    case LighterState::Closing: return "Closing";
    }
    return "Unknown";
}
