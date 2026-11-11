#include "Lighter.h"
#include "DemoController.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <array>
#include <random>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void near(float actual, float expected, const char* message) {
    check(std::abs(actual - expected) < 0.001f, message);
}

void cycle(float dt) {
    Lighter lighter;
    check(!lighter.closeLid(), "Closed must reject duplicate close");
    check(lighter.openLid(), "Open request rejected");
    check(!lighter.openLid(), "Opening must reject duplicate open");
    for (float t = 0; t < 0.6f; t += dt) lighter.update(dt);
    check(lighter.state() == LighterState::Open, "Opening did not finish");
    near(lighter.lidAngle(), 115.0f, "Wrong open endpoint");
    check(lighter.closeLid(), "Close request rejected");
    for (float t = 0; t < 0.6f; t += dt) lighter.update(dt);
    check(lighter.state() == LighterState::Closed, "Closing did not finish");
    near(lighter.lidAngle(), 0.0f, "Wrong closed endpoint");
}

void invariant(const Lighter& lighter) {
    check(lighter.lidAngle() >= 0.0f && lighter.lidAngle() <= 115.0f, "Lid out of range");
    check(lighter.flameScale() >= 0.0f && lighter.flameScale() <= 1.0f, "Flame out of range");
    if (lighter.flameScale() > 0.0f) near(lighter.lidAngle(), 115.0f, "Flame under moving lid");
    const auto state = lighter.state();
    if (state == LighterState::Closed || state == LighterState::Closing || state == LighterState::Opening || state == LighterState::Open) {
        near(lighter.flameScale(), 0.0f, "Flame outside combustion states");
    }
    if (state == LighterState::Closed) near(lighter.lidAngle(), 0.0f, "Closed angle mismatch");
    if (state == LighterState::Burning) near(lighter.flameScale(), 1.0f, "Burning scale mismatch");
}

Lighter inState(LighterState state) {
    Lighter lighter;
    if (state == LighterState::Closed) return lighter;
    lighter.openLid();
    if (state == LighterState::Opening) { lighter.update(0.2f); return lighter; }
    lighter.update(0.5f);
    if (state == LighterState::Open) return lighter;
    if (state == LighterState::Closing) { lighter.closeLid(); lighter.update(0.2f); return lighter; }
    lighter.ignite();
    if (state == LighterState::Igniting) { lighter.update(0.2f); return lighter; }
    lighter.update(0.4f);
    if (state == LighterState::Extinguishing) lighter.extinguish();
    return lighter;
}

void allInputs() {
    const std::array<LighterState, 7> states{LighterState::Closed, LighterState::Opening, LighterState::Open,
        LighterState::Igniting, LighterState::Burning, LighterState::Extinguishing, LighterState::Closing};
    const bool accepted[7][4] = {{true,false,false,false}, {false,true,false,false}, {false,true,true,false},
        {false,true,false,true}, {false,true,false,true}, {true,true,false,false}, {true,false,false,false}};
    using Action = bool (Lighter::*)();
    const std::array<Action, 4> actions{&Lighter::openLid, &Lighter::closeLid, &Lighter::ignite, &Lighter::extinguish};
    for (std::size_t s = 0; s < states.size(); ++s) {
        for (std::size_t a = 0; a < actions.size(); ++a) {
            auto lighter = inState(states[s]);
            check((lighter.*actions[a])() == accepted[s][a], "Input matrix differs from design");
            invariant(lighter);
            lighter.update(2.0f);
            invariant(lighter);
        }
        auto lighter = inState(states[s]);
        lighter.reset(); lighter.update(2.0f);
        check(lighter.state() == LighterState::Closed, "Reset failed from a state");
        check(!lighter.takeSparkEvent() && !lighter.takeSmokeEvent(), "Reset left pending effects");
        near(lighter.wheelAngle(), 0.0f, "Reset left wheel rotation");
    }
}

void ignition() {
    auto lighter = inState(LighterState::Open);
    lighter.ignite();
    lighter.update(0.079f);
    check(!lighter.takeSparkEvent(), "Spark triggered early");
    check(lighter.wheelAngle() > 0.0f, "Wheel did not rotate before spark");
    lighter.update(0.002f);
    check(lighter.takeSparkEvent() && !lighter.takeSparkEvent(), "Spark event must fire once");
    near(lighter.flameScale(), 0.0f, "Flame appeared before spark");
    lighter.update(0.18f);
    check(lighter.flameScale() > 0.0f && lighter.flameScale() < 1.0f, "Flame did not grow");
    const float wheel = lighter.wheelAngle();
    lighter.update(0.3f);
    near(lighter.wheelAngle(), wheel, "Wheel spun after 0.18 seconds");
    check(!lighter.takeSparkEvent(), "Spark repeated");
    check(!lighter.ignite(), "Repeated ignition accepted");
    lighter.closeLid();
    lighter.update(0.15f);
    check(lighter.state() == LighterState::Extinguishing, "Lid closed while burning");
    near(lighter.lidAngle(), 115.0f, "Lid moved before flame was out");
    lighter.openLid();
    lighter.update(0.2f);
    check(lighter.state() == LighterState::Open, "O did not cancel deferred close");

    for (float time : {0.0f, 0.04f, 0.079f, 0.081f, 0.149f, 0.151f, 0.25f, 0.399f}) {
        for (bool close : {false, true}) {
            lighter = inState(LighterState::Open);
            lighter.ignite(); lighter.update(time);
            if (close) lighter.closeLid(); else lighter.extinguish();
            check(!lighter.takeSparkEvent(), "Cancelled ignition left pending spark");
            const float stoppedWheel = lighter.wheelAngle();
            lighter.update(2.0f);
            near(lighter.wheelAngle(), stoppedWheel, "Cancelled ignition kept spinning");
            near(lighter.flameScale(), 0.0f, "Cancelled ignition reignited");
            check(lighter.state() == (close ? LighterState::Closed : LighterState::Open), "Wrong interrupted endpoint");
        }
    }
    lighter = inState(LighterState::Burning);
    lighter.extinguish(); lighter.closeLid(); lighter.update(0.81f);
    check(lighter.state() == LighterState::Closed, "Large step lost extinguish/close time");
    lighter = inState(LighterState::Open);
    lighter.ignite(); lighter.update(0.8f);
    check(lighter.takeSparkEvent() && lighter.state() == LighterState::Burning, "Large step lost spark");
}

void demos() {
    for (float dt : {1.0f/30, 1.0f/60, 1.0f/144, 0.25f}) {
        Lighter lighter;
        DemoController demo;
        demo.start(lighter);
        int burns = 0;
        auto previous = lighter.state();
        for (float t = 0.0f; t < 20.0f && demo.active(); t += dt) {
            demo.update(dt, lighter); lighter.update(dt);
            if (lighter.state() == LighterState::Burning && previous != LighterState::Burning) ++burns;
            previous = lighter.state(); invariant(lighter);
        }
        check(!demo.active() && lighter.state() == LighterState::Closed && burns == 2, "Demo sequence incomplete");
        demo.start(lighter); demo.update(1.1f, lighter); demo.stop(); lighter.update(1.0f);
        demo.update(20.0f, lighter);
        check(lighter.state() == LighterState::Open, "Stopped demo continued");
    }
}

void randomInputs() {
    std::mt19937 random(42);
    Lighter lighter;
    for (int i = 0; i < 100000; ++i) {
        switch (random() % 6) {
        case 0: lighter.openLid(); break;
        case 1: lighter.closeLid(); break;
        case 2: lighter.ignite(); break;
        case 3: lighter.extinguish(); break;
        case 4: lighter.reset(); break;
        default: break;
        }
        invariant(lighter);
        lighter.update(static_cast<float>(random() % 100) / 100.0f);
        invariant(lighter);
        lighter.takeSparkEvent(); lighter.takeSmokeEvent();
    }
}
}

int main() {
    try {
        cycle(1.0f / 30.0f);
        cycle(1.0f / 60.0f);
        cycle(1.0f / 144.0f);
        cycle(0.8f);
        allInputs();
        ignition();
        demos();
        randomInputs();

        Lighter lighter;
        lighter.openLid();
        lighter.update(0.2f);
        const float angle = lighter.lidAngle();
        check(lighter.closeLid(), "Reverse close rejected");
        near(lighter.lidAngle(), angle, "Reverse operation jumped");
        lighter.update(0.02f);
        check(lighter.lidAngle() < angle, "Reverse operation went in wrong direction");
        const float closingAngle = lighter.lidAngle();
        check(lighter.openLid(), "Reverse open rejected");
        near(lighter.lidAngle(), closingAngle, "Reverse opening jumped");
        lighter.update(-1.0f);
        near(lighter.lidAngle(), closingAngle, "Negative time changed angle");
        lighter.update(1.0f);
        check(lighter.state() == LighterState::Open, "Reverse opening did not finish");

        for (int i = 0; i < 1000; ++i) {
            lighter.closeLid();
            lighter.update(0.2f);
            lighter.openLid();
            lighter.update(0.1f);
            lighter.reset();
            check(lighter.state() == LighterState::Closed, "Reset failed");
            near(lighter.lidAngle(), 0.0f, "Reset left stale angle");
            lighter.update(10.0f);
            near(lighter.lidAngle(), 0.0f, "Reset left stale animation");
            lighter.openLid();
            lighter.update(1.0f);
        }
        lighter.reset();
        lighter.openLid();
        check(lighter.closeLid(), "Immediate reversal rejected");
        lighter.update(0.0f);
        check(lighter.state() == LighterState::Closed, "Zero-length animation stuck");
        std::cout << "All lighter state tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
