#pragma once

class Lighter;

class DemoController {
public:
    void start(Lighter& lighter);
    void stop() { step_ = 0; elapsed_ = 0.0f; }
    void update(float dt, Lighter& lighter);
    bool active() const { return step_ != 0; }
    int step() const { return step_; }

private:
    void next() { ++step_; elapsed_ = 0.0f; }
    int step_ = 0;
    float elapsed_ = 0.0f;
};
