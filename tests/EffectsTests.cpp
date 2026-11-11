#include "Simulation.h"
#include "Camera.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        ParticleSystem particles;
        particles.burst(); particles.smoke();
        check(particles.activeCount() == 88, "Unexpected pool capacity");
        const auto initial = particles.sparks()[0];
        particles.update(0.02f);
        const auto moved = particles.sparks()[0];
        const glm::vec3 expected = initial.position + initial.velocity * 0.02f + glm::vec3(0, -5, 0) * 0.0002f;
        check(glm::length(moved.position - expected) < 1e-5f, "Particle integration mismatch");
        particles.update(2.0f);
        check(particles.activeCount() == 0, "Expired particles not reclaimed");
        for (int i = 0; i < 1000; ++i) { particles.burst(); particles.smoke(); }
        check(particles.activeCount() == 88, "Particle pool grew");
        particles.reset(); check(particles.activeCount() == 0, "Particle reset failed");

        for (float dt : {1.0f/30, 1.0f/60, 1.0f/144, 0.8f}) {
            Simulation sim;
            sim.lighter.openLid(); sim.update(0.51f);
            sim.lighter.ignite(); sim.update(0.10f);
            check(sim.particles.activeCount() > 0 && sim.lighter.flameScale() == 0, "Spark/flame order incorrect");
            sim.lighter.closeLid(); sim.update(1.0f);
            check(sim.lighter.state() == LighterState::Closed && sim.particles.activeCount() == 0, "Cancelled ignition leaked particles");
            sim.toggleDemo();
            for (float t = 0; t < 12.0f && sim.demo.active(); t += dt) sim.update(dt);
            check(!sim.demo.active() && sim.lighter.state() == LighterState::Closed, "Simulation demo failed");
            sim.reset();
            check(sim.time() == 0 && sim.particles.activeCount() == 0 && !sim.demo.active(), "Global reset incomplete");
        }

        Simulation sim;
        int demos = 0;
        for (int i = 0; i < 600; ++i) {
            if (!sim.demo.active()) { sim.toggleDemo(); ++demos; }
            sim.update(1.0f);
            check(sim.particles.activeCount() <= 88, "Simulation pool grew");
            check(sim.lighter.flameScale() == 0 || sim.lighter.lidAngle() == 115, "Unsafe flame");
        }
        check(demos > 50, "Accelerated endurance failed");

        Camera camera;
        const auto defaultPosition = camera.position();
        camera.orbit(36000, 100000);
        camera.zoom(100000);
        check(std::isfinite(camera.view()[0][0]), "Camera pole singularity");
        camera.pan(100000, -100000); camera.zoom(-100000);
        check(std::isfinite(camera.position().x), "Camera pan/zoom overflow");
        camera.reset();
        check(glm::length(camera.position() - defaultPosition) < 0.0001f, "Camera reset failed");
        std::cout << "Particle, simulation, camera and accelerated 600-second tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
