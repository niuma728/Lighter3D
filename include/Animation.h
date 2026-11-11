#pragma once

#include <algorithm>

class Animation {
public:
    void start(float from, float to, float duration) {
        from_ = from;
        to_ = to;
        duration_ = duration;
        elapsed_ = 0.0f;
    }

    float update(float dt) {
        elapsed_ = std::min(elapsed_ + std::max(dt, 0.0f), duration_);
        const float u = duration_ > 0.0f ? elapsed_ / duration_ : 1.0f;
        return from_ + (to_ - from_) * u * u * (3.0f - 2.0f * u);
    }

    bool finished() const { return elapsed_ >= duration_; }
    float remaining() const { return duration_ - elapsed_; }

private:
    float from_ = 0.0f;
    float to_ = 0.0f;
    float duration_ = 0.0f;
    float elapsed_ = 0.0f;
};
