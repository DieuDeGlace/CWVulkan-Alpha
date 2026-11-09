// Engine GPU routing diagnostic. No owner assets, native code or native pixel oracle.
#include "Renderer.h"
#include "SectorLights.h"
#include "WorldItems.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {

ModelMesh quad() {
    ModelMesh mesh;
    for (glm::vec3 p : {glm::vec3(4, 4, 0), glm::vec3(12, 4, 0),
                       glm::vec3(12, 12, 0), glm::vec3(4, 12, 0)})
        mesh.vertices.push_back({p, {0, 0, 1}, {1, 1, 1, 0}});
    // Both windings make this probe independent of raster front-face convention.
    mesh.indices = {0, 1, 2, 0, 2, 3, 2, 1, 0, 3, 2, 0};
    return mesh;
}

terrain_lights::Candidate light(glm::dvec3 at, float radius, glm::vec3 colour) {
    auto fixed = terrain_lights::fromBlocks(at);
    if (!fixed)
        throw std::runtime_error("Probe coordinate is not representable");
    return {*fixed, colour, radius, 0};
}

using RGB = std::array<unsigned char, 3>;
std::array<RGB, 2> sample(const std::filesystem::path &path, const FrameConstants &frame) {
    std::ifstream file(path, std::ios::binary);
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file), {}};
    int width = 0, height = 0, channels = 0;
    unsigned char *image = stbi_load_from_memory(bytes.data(), int(bytes.size()), &width, &height, &channels, 4);
    if (!image)
        throw std::runtime_error("Cannot decode probe capture");
    std::array<RGB, 2> result{};
    const std::array centres{glm::vec3(8, 8, 0), glm::vec3(40, 8, 0)};
    for (size_t i = 0; i < centres.size(); ++i) {
        const glm::vec4 clip = frame.viewProjection * glm::vec4(centres[i], 1);
        const glm::vec2 uv = glm::vec2(clip) / clip.w * .5f + .5f;
        const int x = int(uv.x * float(width)), y = int(uv.y * float(height));
        if (clip.w <= 0 || x < 0 || y < 0 || x >= width || y >= height) {
            stbi_image_free(image);
            throw std::runtime_error("Probe sample is outside the capture");
        }
        const size_t offset = (size_t(y) * size_t(width) + size_t(x)) * 4;
        std::copy_n(image + offset, 3, result[i].begin());
    }
    stbi_image_free(image);
    return result;
}

void report(const char *label, const std::array<RGB, 2> &pixels) {
    std::cout << label;
    for (const auto &p : pixels)
        std::cout << " (" << int(p[0]) << ',' << int(p[1]) << ',' << int(p[2]) << ')';
    std::cout << '\n';
}

bool black(const std::array<RGB, 2> &pixels) {
    for (const auto &p : pixels)
        for (unsigned char channel : p)
            if (channel > 4)
                return false;
    return true;
}

void run(GLFWwindow *window, const std::filesystem::path &output) {
    Renderer renderer;
    AntiAliasing aa;
    aa.taa = aa.smaa = false;
    ShadowSettings shadows;
    shadows.enabled = false;
    shadows.resolution = 512;
    std::string error;
    if (!renderer.init(window, false, aa, shadows, &error))
        throw std::runtime_error(error);
    const MeshId left = renderer.addMesh(quad()), right = renderer.addMesh(quad());
    const std::vector<DrawItem> draws{
        {left, glm::mat4(1), false},
        {right, glm::translate(glm::mat4(1), glm::vec3(32, 0, 0)), false}};
    const std::array<terrain_lights::Chunk, 2> chunks{{{{0, 0}, left}, {{1, 0}, right}}};
    Camera camera;
    camera.eye = {24, -32, 32};
    camera.pitchDegrees = glm::degrees(std::atan(32.f / 40.f));
    camera.aspect = float(renderer.size().width) / float(renderer.size().height);
    Scene scene;
    scene.fogDistance = 0;
    // Global fallback is conspicuously green. Terrain rows must override it.
    scene.lights.push_back({{24, 8, 4}, 100, {0, 1, 0}});
    MoonSettings moon;
    moon.enabled = false;
    auto frame = makeFrame(camera, scene, shadows, moon);
    frame.material = {1, 1, 1, 0};
    frame.params.x = frame.params.z = frame.params.w = 0;
    frame.moonFront = frame.moonBack = frame.nightAmbient = glm::vec4(0);
    terrain_lights::Selector selector;
    const auto capture = [&](const char *name, std::span<const terrain_lights::Selection> rows) {
        renderer.setTerrainLights(rows);
        const auto path = output / (std::string(name) + ".png");
        for (int i = 0; i < 3; ++i) {
            glfwPollEvents();
            if (!renderer.draw(frame, false, draws, i == 2 ? path : std::filesystem::path{}))
                throw std::runtime_error("Vulkan probe draw failed");
        }
        if (renderer.drawnItems() != 2)
            throw std::runtime_error("Probe quads were not both drawn");
        const auto pixels = sample(path, frame);
        report(name, pixels);
        return pixels;
    };
    const auto requireColours = [](const std::array<RGB, 2> &coloured) {
        if (coloured[0][0] < 40 || coloured[0][1] > 4 || coloured[0][2] > 4 ||
            coloured[1][2] < 40 || coloured[1][0] > 4 || coloured[1][1] > 4)
            throw std::runtime_error("Terrain storage rows did not route red/blue lights");
    };
    const std::vector colours{light({8, 8, 4}, 16, {1, 0, 0}), light({40, 8, 4}, 16, {0, 0, 1})};
    requireColours(capture("chunk-colours", selector.rebuild(colours, chunks, {})));
    // Force a second allocation/descriptor update after earlier frames used the
    // buffer, and exercise nontrivial storage row offsets. Mesh zero is unused.
    std::vector<terrain_lights::Selection> grown(6);
    const auto &selected = selector.rebuild(colours, chunks, {});
    grown.insert(grown.end(), selected.begin(), selected.end());
    requireColours(capture("grown-buffer", grown));
    if (!black(capture("cleared-rows", selector.rebuild({}, chunks, {}))))
        throw std::runtime_error("Cleared terrain rows retained or fell back to lights");
    std::vector<terrain_lights::Candidate> disabledPrefix;
    for (size_t i = 0; i < terrain_lights::CAPACITY; ++i)
        disabledPrefix.push_back(light({8, 8, 4}, i < terrain_lights::UPLOADED ? 0.f : 16.f, {1, 0, 0}));
    if (!black(capture("fixed-ten-prefix", selector.rebuild(disabledPrefix, chunks, {}))))
        throw std::runtime_error("Shader compacted disabled slots or consumed beyond slot ten");
    terrain_lights::FadingSources timed;
    timed.append({colours[0].position, {1, 0, 0}, 16, 0, 400});
    timed.append({colours[1].position, {0, 0, 1}, 16, 0, 800});
    const auto captureTimed = [&](const char *name) {
        std::vector<terrain_lights::Candidate> candidates;
        for (const auto &source : timed.entries())
            candidates.push_back({source.position, source.fadedColour(), source.radius, 0});
        return capture(name, selector.rebuild(candidates, chunks, {}));
    };
    timed.advance(400);
    const auto faded = captureTimed("timed-equality");
    if (faded[0][0] > 4 || faded[0][1] > 4 || faded[0][2] > 4 ||
        faded[1][0] > 4 || faded[1][1] > 4 || faded[1][2] < 50 || faded[1][2] > 120)
        throw std::runtime_error("Timed source equality/half-fade did not reach the terrain shader");
    timed.advance(401);
    if (!black(captureTimed("timed-expired")))
        throw std::runtime_error("Expired sources retained GPU contributions");
    terrain_lights::SectorSources sectorSources;
    worldgen::Zone sector;
    worldgen::StaticObject lamp;
    lamp.kind = 50;
    lamp.x = lamp.y = 8 * 65536;
    lamp.scale[2] = 4;
    sector.statics.push_back(lamp);
    sectorSources.update(sector);
    WorldItems liveItems;
    WorldItem candle;
    candle.item[0] = 18;
    candle.item[1] = 1;
    candle.position[0] = 40 * 65536;
    candle.position[1] = 8 * 65536;
    candle.position[2] = 65536;
    liveItems.replace(0, 0, {candle});
    const terrain_lights::ViewAdmission sourceView(*terrain_lights::fromBlocks(glm::dvec3(camera.eye)), {},
                                                  camera.viewPlanes(), 200);
    const auto captureSector = [&](const char *name) {
        std::vector<terrain_lights::Candidate> candidates;
        sectorSources.append(candidates, liveItems, sourceView, {24 * 65536, 8 * 65536, 0}, 0);
        return capture(name, selector.rebuild(candidates, chunks, {}));
    };
    const auto gathered = captureSector("sector-sources");
    if (gathered[0][0] < 150 || gathered[0][1] < 100 || gathered[0][2] < 80 ||
        gathered[0][0] <= gathered[0][1] || gathered[0][1] <= gathered[0][2] ||
        gathered[1][0] > 4 || gathered[1][1] < 50 || gathered[1][2] < 10 || gathered[1][1] < 2 * gathered[1][2])
        throw std::runtime_error("Gathered static/item sources did not reach their terrain rows");
    candle.hideMs = 1;
    liveItems.replace(0, 0, {candle});
    const auto hidden = captureSector("sector-hidden");
    if (hidden[0][0] < 150 || hidden[1][0] > 4 || hidden[1][1] > 4 || hidden[1][2] > 4)
        throw std::runtime_error("Live hidden item retained its GPU contribution");
    candle.hideMs = 0;
    liveItems.replace(0, 0, {candle});
    sectorSources.remove(0, 0);
    if (!black(captureSector("sector-unloaded")))
        throw std::runtime_error("Unloaded sector retained GPU static/item contributions");
    const auto fallback = capture("global-fallback", {});
    for (const auto &p : fallback)
        if (p[1] < 40 || p[0] > 4 || p[2] > 4)
            throw std::runtime_error("Removed terrain rows did not restore model fallback");
    std::cout << "PASS: GPU terrain routing, growth, clearing, ten-prefix, timed fading, sector lifecycle and fallback\n";
}

} // namespace

int main(int argc, char **argv) {
    const std::filesystem::path output = argc > 1 ? argv[1] : "terrain-light-render-probe";
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Terrain light routing probe", nullptr, nullptr);
    int result = 0;
    try {
        if (!window)
            throw std::runtime_error("Cannot create probe window");
        std::filesystem::create_directories(output);
        run(window, output);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        result = 1;
    }
    if (window)
        glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
