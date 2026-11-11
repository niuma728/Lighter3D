#include "Camera.h"
#include "Lighter.h"
#include "Renderer.h"
#include "Simulation.h"

#include <GLFW/glfw3.h>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {
struct Input {
    Simulation simulation;
    Camera camera;
    bool dragging = false;
    bool panning = false;
    double lastX = 0;
    double lastY = 0;
    std::string notice;
};

Input& input(GLFWwindow* window) {
    return *static_cast<Input*>(glfwGetWindowUserPointer(window));
}

void key(GLFWwindow* window, int code, int, int action, int) {
    if (action != GLFW_PRESS) return;
    auto& controls = input(window);
    bool accepted = true;
    auto& simulation = controls.simulation;
    auto& lighter = simulation.lighter;
    if (code == GLFW_KEY_O || code == GLFW_KEY_C || code == GLFW_KEY_F || code == GLFW_KEY_E) simulation.demo.stop();
    switch (code) {
    case GLFW_KEY_O: accepted = lighter.openLid(); break;
    case GLFW_KEY_C: accepted = lighter.closeLid(); break;
    case GLFW_KEY_F: accepted = lighter.ignite(); break;
    case GLFW_KEY_E: accepted = lighter.extinguish(); break;
    case GLFW_KEY_SPACE: simulation.toggleDemo(); break;
    case GLFW_KEY_R:
        simulation.reset();
        controls.camera.reset();
        controls.dragging = false;
        controls.panning = false;
        break;
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    default: return;
    }
    controls.notice = accepted ? "" : "Request ignored in current state";
    if (code == GLFW_KEY_F && !accepted) {
        switch (lighter.state()) {
        case LighterState::Closed: case LighterState::Closing: controls.notice = "Open the lid before igniting"; break;
        case LighterState::Opening: controls.notice = "Wait for the lid to open"; break;
        case LighterState::Extinguishing: controls.notice = "Wait for extinguishing to finish"; break;
        default: controls.notice = "Already igniting or burning"; break;
        }
    }
    simulation.update(0.0f);
}

void mouseButton(GLFWwindow* window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT && button != GLFW_MOUSE_BUTTON_RIGHT) return;
    auto& controls = input(window);
    if (button == GLFW_MOUSE_BUTTON_LEFT) controls.dragging = action == GLFW_PRESS;
    else controls.panning = action == GLFW_PRESS;
    glfwGetCursorPos(window, &controls.lastX, &controls.lastY);
}

void cursor(GLFWwindow* window, double x, double y) {
    auto& controls = input(window);
    if (controls.dragging) {
        controls.camera.orbit(static_cast<float>(x - controls.lastX), static_cast<float>(y - controls.lastY));
    }
    if (controls.panning) controls.camera.pan(static_cast<float>(x - controls.lastX), static_cast<float>(y - controls.lastY));
    controls.lastX = x;
    controls.lastY = y;
}

std::filesystem::path resourceDirectory(const char* executable) {
#ifdef _WIN32
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) throw std::runtime_error("Cannot locate executable directory");
    buffer.resize(length);
    (void)executable;
    return std::filesystem::path(buffer).parent_path();
#else
    return std::filesystem::canonical(executable).parent_path();
#endif
}

// BMP keeps smoke-test captures dependency-free; OpenGL supplies bottom-up BGR pixels.
void screenshot(const std::filesystem::path& path, int width, int height) {
    const int stride = (width * 3 + 3) & ~3;
    std::vector<unsigned char> pixels(static_cast<std::size_t>(stride) * height);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());
    std::array<unsigned char, 54> header{};
    const auto put = [&header](int offset, std::uint32_t value) {
        for (int i = 0; i < 4; ++i) header[offset + i] = static_cast<unsigned char>(value >> (8 * i));
    };
    header[0] = 'B'; header[1] = 'M';
    put(2, static_cast<std::uint32_t>(54 + pixels.size()));
    put(10, 54); put(14, 40); put(18, width); put(22, height);
    header[26] = 1; header[28] = 24;
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(header.data()), header.size());
    file.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
    if (!file) throw std::runtime_error("Cannot save screenshot: " + path.string());
}

void validate(const Simulation& simulation) {
    const auto& lighter = simulation.lighter;
    if (lighter.flameScale() > 0.0f && lighter.lidAngle() != Lighter::openAngle) {
        throw std::runtime_error("Invariant failed: flame with moving/closed lid");
    }
    if (simulation.particles.activeCount() > ParticleSystem::sparkCapacity + ParticleSystem::smokeCapacity) {
        throw std::runtime_error("Particle pool overflow");
    }
    const auto error = glGetError();
    if (error != GL_NO_ERROR) throw std::runtime_error("OpenGL error: " + std::to_string(error));
}

void run(GLFWwindow* window, const std::filesystem::path& resources, const std::string& mode) {
    Renderer renderer(resources);
    Input controls;
    glfwSetWindowUserPointer(window, &controls);
    glfwSetKeyCallback(window, key);
    glfwSetMouseButtonCallback(window, mouseButton);
    glfwSetCursorPosCallback(window, cursor);
    glfwSetScrollCallback(window, [](GLFWwindow* w, double, double y) { input(w).camera.zoom(static_cast<float>(y)); });
    glfwSetWindowFocusCallback(window, [](GLFWwindow* w, int) { input(w).dragging = false; input(w).panning = false; });
    std::cout << "O: open | C: close | F: ignite | E: extinguish | Space: demo | R: reset\n"
              << "Left drag: orbit | Right drag: pan | Wheel: zoom | Esc: exit\n";
    auto& simulation = controls.simulation;
    auto& lighter = simulation.lighter;

    if (mode == "--smoke-test") {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        const auto capture = [&](const char* name) {
            renderer.render(simulation, controls.camera, controls.notice, width, height);
            screenshot(resources / (std::string(name) + ".bmp"), width, height);
            validate(simulation);
        };
        key(window, GLFW_KEY_F, 0, GLFW_PRESS, 0);
        if (controls.notice.empty() || lighter.state() != LighterState::Closed) throw std::runtime_error("Closed ignition input failure");
        key(window, GLFW_KEY_O, 0, GLFW_REPEAT, 0);
        if (lighter.state() != LighterState::Closed) throw std::runtime_error("Repeated key was accepted");
        key(window, GLFW_KEY_R, 0, GLFW_PRESS, 0);
        capture("closed");
        key(window, GLFW_KEY_O, 0, GLFW_PRESS, 0);
        simulation.update(0.51f);
        capture("open");
        controls.camera.orbit(-100.0f, -60.0f);
        controls.camera.zoom(7.0f);
        controls.camera.pan(-71.4286f, 214.2857f);
        capture("mechanism");
        controls.camera.reset();
        key(window, GLFW_KEY_F, 0, GLFW_PRESS, 0);
        simulation.update(0.10f);
        capture("sparks");
        simulation.update(0.16f);
        capture("igniting");
        simulation.update(0.30f);
        capture("burning");
        controls.camera.orbit(560.0f, -100.0f);
        capture("rear");
        controls.camera.reset();
        controls.camera.orbit(200.0f, 160.0f);
        controls.camera.zoom(-2.0f);
        capture("top");
        controls.camera.orbit(0.0f, -400.0f);
        capture("bottom");
        controls.camera.reset();
        lighter.extinguish();
        simulation.update(0.15f);
        capture("extinguishing");
        simulation.update(0.25f);
        capture("smoke");
        lighter.ignite();
        simulation.update(0.41f);
        lighter.closeLid();
        simulation.update(0.31f);
        if (lighter.flameScale() != 0.0f || lighter.state() != LighterState::Closing) throw std::runtime_error("Close-before-extinguish failure");
        simulation.update(0.24f);
        capture("closing");
        simulation.update(0.3f);
        key(window, GLFW_KEY_SPACE, 0, GLFW_PRESS, 0);
        simulation.update(3.0f);
        key(window, GLFW_KEY_E, 0, GLFW_PRESS, 0);
        if (simulation.demo.active()) throw std::runtime_error("Manual input did not stop demo");
        key(window, GLFW_KEY_R, 0, GLFW_PRESS, 0);
        key(window, GLFW_KEY_SPACE, 0, GLFW_PRESS, 0);
        simulation.update(12.0f);
        if (simulation.demo.active() || lighter.state() != LighterState::Closed) throw std::runtime_error("Demo did not finish");
        controls.camera.orbit(40.0f, 30.0f);
        controls.camera.pan(20.0f, 30.0f);
        controls.camera.zoom(2.0f);
        key(window, GLFW_KEY_R, 0, GLFW_PRESS, 0);
        if (glm::length(controls.camera.position() - Camera{}.position()) > 0.001f || simulation.particles.activeCount() != 0) {
            throw std::runtime_error("Global reset failed");
        }
        glfwSetWindowSize(window, 800, 600);
        glfwPollEvents();
        glfwGetFramebufferSize(window, &width, &height);
        capture("resized");
        std::cout << "OpenGL smoke test passed: all parts, ignition, flame, smoke, views, resize, demo\n";
        return;
    }

    if (mode == "--benchmark") {
        lighter.openLid(); simulation.update(0.51f);
        lighter.ignite(); simulation.update(0.41f);
        for (int i = 0; i < 60; ++i) {
            simulation.update(1.0f / 60.0f);
            renderer.render(simulation, controls.camera, "BENCHMARK 1920 X 1080", 1920, 1080);
            glfwSwapBuffers(window);
        }
        glFinish();
        const double begin = glfwGetTime();
        constexpr int frames = 600;
        for (int i = 0; i < frames; ++i) {
            simulation.update(1.0f / 60.0f);
            controls.camera.orbit(1.0f, 0.0f);
            renderer.render(simulation, controls.camera, "BENCHMARK 1920 X 1080", 1920, 1080);
            glfwSwapBuffers(window);
        }
        glFinish();
        validate(simulation);
        std::cout << "Benchmark 1920x1080: " << frames / (glfwGetTime() - begin) << " FPS, " << frames << " burning frames\n";
        return;
    }

    double lastTime = glfwGetTime();
    const double started = lastTime;
    double fpsTime = 0.0;
    float fps = 0.0f;
    int frameCount = 0, completedDemos = 0, nextReport = 60;
    const bool endurance = mode == "--endurance-test";
    if (endurance) simulation.toggleDemo();
    std::string previousTitle;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0 || glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            controls.dragging = false;
            glfwWaitEventsTimeout(0.1);
            lastTime = glfwGetTime();
            continue;
        }
        // Treat debugger/suspend gaps as a pause; normal animation uses elapsed seconds.
        if (dt < 1.0f) simulation.update(dt);
        if (endurance) {
            if (!simulation.demo.active()) { ++completedDemos; simulation.toggleDemo(); }
            controls.camera.orbit(dt * 24.0f, dt * std::sin(static_cast<float>(now) * 0.3f) * 5.0f);
            validate(simulation);
            if (now - started >= nextReport) {
                std::cout << "Endurance " << nextReport << "s, demos=" << completedDemos
                          << ", particles=" << simulation.particles.activeCount() << std::endl;
                nextReport += 60;
            }
            if (now - started >= 600.0) {
                std::cout << "Endurance passed: 600 seconds, " << completedDemos << " complete demos" << std::endl;
                return;
            }
        }
        ++frameCount;
        fpsTime += dt;
        if (fpsTime >= 1.0) { fps = static_cast<float>(frameCount / fpsTime); frameCount = 0; fpsTime = 0.0; }
        std::string title = std::string("Lighter3D | ") + lighter.stateName()
            + " | O Open / C Close / F Ignite / E Extinguish / Space Demo / R Reset";
        if (!controls.notice.empty()) title += " | " + controls.notice;
        if (title != previousTitle) {
            glfwSetWindowTitle(window, title.c_str());
            previousTitle = title;
        }
        renderer.render(simulation, controls.camera, controls.notice, width, height, fps);
        glfwSwapBuffers(window);
        if (endurance) glfwWaitEventsTimeout(1.0 / 60.0);
    }
}
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "";
    if (argc > 2 || (!mode.empty() && mode != "--smoke-test" && mode != "--endurance-test" && mode != "--benchmark")) {
        std::cerr << "Usage: Lighter3D [--smoke-test | --endurance-test | --benchmark]\n";
        return 1;
    }
    glfwSetErrorCallback([](int code, const char* message) { std::cerr << "GLFW " << code << ": " << message << '\n'; });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    if (!mode.empty()) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    const bool fullHD = mode == "--endurance-test" || mode == "--benchmark";
    GLFWwindow* window = glfwCreateWindow(fullHD ? 1920 : 1100, fullHD ? 1080 : 800, "Lighter3D", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(mode.empty() ? 1 : 0);
    glfwSetWindowSizeLimits(window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
    int result = 0;
    try {
        if (!gladLoadGL(glfwGetProcAddress)) throw std::runtime_error("Cannot load OpenGL 3.3 functions");
        if (!GLAD_GL_VERSION_3_3) throw std::runtime_error("OpenGL 3.3 Core is required");
        std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\nRenderer: " << glGetString(GL_RENDERER) << '\n';
        run(window, resourceDirectory(argv[0]), mode);
    } catch (const std::exception& error) {
        std::cerr << "Lighter3D: " << error.what() << '\n';
        result = 1;
    }
    // All renderer resources leave scope before the context is destroyed.
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
