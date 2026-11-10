// Engine UI diagnostic. Reads owned fonts/interfaces, writes only a new capture
// folder. Captures verify this extension, not original Alpha pixel equivalence.
#include "Renderer.h"
#include "Install.h"
#include "ui/Screens.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void run(GLFWwindow *window, const std::filesystem::path &install, const std::filesystem::path &output) {
    Renderer renderer;
    AntiAliasing aa;
    aa.taa = aa.smaa = false;
    ShadowSettings shadows;
    shadows.enabled = false;
    shadows.resolution = 512;
    std::string error;
    if (!renderer.init(window, false, aa, shadows, &error))
        throw std::runtime_error(error);
    ui::Interface screen;
    if (!screen.load(install, &error))
        throw std::runtime_error(error);
    MoonSettings moon;
    moon.enabled = false;
    const auto capture = [&](const char *name) {
        if (!screen.openControllerKeyboard())
            throw std::runtime_error("Diagnostic edit target was unavailable");
        for (int frameIndex = 0; frameIndex < 4; frameIndex++) {
            glfwPollEvents();
            const auto size = renderer.size();
            const glm::vec2 pixels{float(size.width), float(size.height)};
            auto ui = screen.draw(pixels, screen.controllerKeyboardFocus(pixels), frameIndex * 0.016);
            if (ui.triangles.empty())
                throw std::runtime_error("Diagnostic UI produced no geometry");
            renderer.setInterface(std::move(ui));
            if (screen.atlas.takeChanged())
                renderer.setInterfaceAtlas(screen.atlas.size(), screen.atlas.size(), screen.atlas.pixels());
            Camera camera;
            camera.aspect = pixels.x / pixels.y;
            Scene scene;
            const auto frame = makeFrame(camera, scene, shadows, moon);
            const auto path = frameIndex == 3 ? output / (std::string(name) + ".png") : std::filesystem::path{};
            if (!renderer.draw(frame, false, {}, path))
                throw std::runtime_error("Vulkan keyboard draw failed");
        }
        std::cout << "Rendered " << name << '\n';
    };
    screen.front = ui::Interface::Front::Creation;
    screen.nameText = u"Controller test";
    capture("character");
    screen.controllerKeyboardPress({1280, 720}, {0, 0}, 2);
    capture("shifted");
    screen.finishControllerKeyboard(false);
    screen.front = ui::Interface::Front::WorldCreation;
    screen.editFocus = 1;
    screen.seedText = u"2412123";
    capture("seed");
    screen.finishControllerKeyboard(false);
    screen.front = ui::Interface::Front::Connect;
    screen.serverText = u"127.0.0.1";
    capture("server");
    screen.finishControllerKeyboard(false);
    screen.front = ui::Interface::Front::None;
    screen.hud = ui::HudState{};
    screen.chatOpen = true;
    screen.chatInput = u"Controller keyboard draft";
    capture("chat");
    glfwSetWindowSize(window, 640, 360);
    capture("small");
    if (!screen.chatSubmitted.empty())
        throw std::runtime_error("Render diagnostic unexpectedly submitted chat");
}
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: ControllerKeyboardRenderProbe <owned Alpha folder> <new output folder>\n";
        return 1;
    }
    const auto install = pathOfUtf8(argv[1]), output = pathOfUtf8(argv[2]);
    if (std::filesystem::exists(output)) {
        std::cerr << "Use a new output folder; existing captures are retained.\n";
        return 1;
    }
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Controller keyboard diagnostic", nullptr, nullptr);
    int result = 0;
    try {
        if (!window)
            throw std::runtime_error("Cannot create diagnostic window");
        std::filesystem::create_directories(output);
        run(window, install, output);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    if (window)
        glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
