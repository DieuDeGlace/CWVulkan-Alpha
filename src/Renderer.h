#pragma once
// The Vulkan side: instance, device, swap chain and the passes of a frame.
//
//   shadow   depth from the sun (or the night light) into the shadow map (Shadows.h)
//   scene    Alpha's frame (alpha-rendering.md §1.3): sky gradient, stars, sun sprite, models,
//            drawn off-screen with a sub-pixel jitter (Renderer.cpp)
//   post     TAA resolve into the history image, then SMAA onto the screen (RendererPost.cpp)
//   ui       the interface, blended over the screen (RendererUi.cpp)
//
// One frame is in flight at a time, which keeps the code plain. Depth and history images
// alternate by frame parity so the TAA pass can read last frame's.
#include "CubModel.h"
#include "Frame.h"
#include "Shadows.h"
#include "ui/Painter.h"
#include <volk.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <string>
#include <span>
#include <vector>

struct GLFWwindow;
class Config;

// [AntiAliasing] in AlphaVulkan.ini. Both are on by default: AlphaVulkan's own addition, as
// Alpha itself only has multisampling (alpha-rendering.md §13).
struct AntiAliasing {
    bool taa = true, smaa = true;
    float historyWeight = .65f, edgeHistoryWeight = .80f, resolveBlur = .12f, jitterScale = .75f;
    void load(const Config &config);
};

// A mesh the renderer holds on the GPU. 0 is "no mesh".
using MeshId = uint32_t;

// How an item is shaded.
enum class Shading : uint8_t {
    World, // terrain, models, plants (alpha-rendering.md §5, §6.2)
    Cloud, // cloud models (§4 cloud row, §6.3)
    Water, // water surfaces (§6.4, or the Omega look), drawn after everything solid
    // The corner map (screens §5.1): tiles, and markers drawn without writing depth.
    Map, MapOutline,
};

// One thing to draw this frame: a mesh placed in the world.
struct DrawItem {
    MeshId mesh = 0;
    glm::mat4 world{1}; // model -> world blocks
    bool castsShadow = true;
    Shading shading = Shading::World;
    // World: rgb times the colour, a times the sun and sky term (a highlighted item); the
    // corner map: its material colour and mode (shaders/minimap.vert).
    glm::vec4 material{1};
    // Materials §1.2: iron, gold, silver and ice items shimmer (world draws only).
    bool shine = false;
};

// One corner of a blended effect triangle (weapon trails): world blocks and an RGBA colour
// (red in the low byte).
struct EffectVertex {
    glm::vec3 position;
    uint32_t rgba;
};

// One corner of an interface triangle (ui/Painter.h).
using UiVertex = ui::Vertex;

class Renderer {
  public:
    bool init(GLFWwindow *window, bool vsync, const AntiAliasing &aa, const ShadowSettings &shadows,
              std::string *error);
    ~Renderer();

    // Hands a mesh to the GPU. It is copied into GPU memory at the start of the next frame;
    // the id is valid at once. Empty meshes get an id too (they draw nothing).
    MeshId addMesh(const ModelMesh &mesh);
    // Frees a mesh once the GPU no longer uses it. Its id may be handed out again.
    void removeMesh(MeshId id);
    // Draws one frame: `items` outside the view (or, for the shadow, outside the shadow map)
    // are skipped. With `capture` set, the finished image is also written there as a PNG.
    bool draw(const FrameConstants &frame, bool stars, const std::vector<DrawItem> &items,
              const std::filesystem::path &capture = {});
    // How many items the last frame drew in the scene and in the shadow map.
    uint32_t drawnItems() const { return lastDrawn; }
    uint32_t shadowItems() const { return lastShadowDrawn; }
    VkExtent2D size() const { return extent; }
    // The interface triangles for the next frames (three vertices each), and the atlas they
    // sample (RGBA, rows top to bottom); the atlas is uploaded on the next frame.
    void setInterface(ui::Frame frame);
    // The effect triangles of the next frame (three vertices each), drawn over the solid world
    // and before the water, without writing depth or culling (trails §1.3).
    void setEffects(std::vector<EffectVertex> vertices) { effects = std::move(vertices); }
    // Stage the terrain's fixed ten-vector prefixes; upload after the prior frame completes.
    void setTerrainLights(std::span<const terrain_lights::Selection> selections);
    // The corner map's meshes for the next frame (screens §5.1), placed in world blocks; the
    // frame's `minimap` matrix takes them to the screen.
    void setMinimap(std::vector<DrawItem> items) { minimapItems = std::move(items); }
    void setInterfaceAtlas(uint32_t width, uint32_t height, std::vector<uint8_t> rgba);
    // Forget the TAA history (after a camera cut or a new model).
    void dropHistory() { historyValid = false; }

  private:
    struct Buffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        void *mapped = nullptr;
    };
    struct Image {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };
    struct PipelineDesc; // see Renderer.cpp

    bool createDevice(std::string *error);
    bool createSwapchain();
    void destroySwapchain();
    bool recreateSwapchain();
    bool createScenePipelines();
    VkPipeline makePipeline(const PipelineDesc &desc);
    uint32_t memoryType(uint32_t bits, VkMemoryPropertyFlags flags) const;
    Buffer makeBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
    Buffer makeDeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
    void recordUploads(VkCommandBuffer cmd);
    void freeRetired();
    bool prepareTerrainLights();
    void freeBuffer(Buffer &b);
    Image makeImage(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);
    void freeImage(Image &image);
    bool saveImage(VkImage image, const std::filesystem::path &file);
    void recordShadow(VkCommandBuffer cmd, const FrameConstants &frame, const std::vector<DrawItem> &items);
    void recordScene(VkCommandBuffer cmd, const FrameConstants &frame, bool stars, const std::vector<DrawItem> &items);
    // RendererWater.cpp: immutable opaque snapshots for the shared Steam surface.
    void updateWaterConstants(const FrameConstants &frame);
    void snapshotWaterBackground(VkCommandBuffer cmd);
    // Binds and draws the items that pass `visible`; returns how many.
    // `only` limits the scene pass to one kind of shading; the shadow pass draws every kind.
    uint32_t drawItems(VkCommandBuffer cmd, const std::vector<DrawItem> &items, const glm::mat4 &clip, bool shadowPass,
                       Shading only = Shading::World);

    // RendererPost.cpp
    bool createPostResources();
    void destroyPostResources();
    bool createPostTargets();
    void destroyPostTargets();
    Image uploadImage(uint32_t w, uint32_t h, VkFormat format, const void *data, size_t bytes);
    void recordPost(VkCommandBuffer cmd, uint32_t imageIndex, glm::vec2 jitter, const glm::mat4 &clipToPrevious);

    // RendererUi.cpp
    bool createUiResources();
    void destroyUiResources();
    void prepareInterface();
    void recordInterface(VkCommandBuffer cmd, uint32_t imageIndex);
    bool createMinimapPipeline();
    void recordMinimap(VkCommandBuffer cmd, uint32_t imageIndex, const FrameConstants &frame);

    GLFWwindow *window = nullptr;
    bool vsync = true;
    AntiAliasing aa;
    ShadowSettings shadowSettings;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkSemaphore imageReady = VK_NULL_HANDLE, renderDone = VK_NULL_HANDLE;
    VkFence frameDone = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat colourFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D extent{};
    std::vector<VkImage> swapImages;
    std::vector<VkImageView> swapViews;

    // Scene pass.
    VkDescriptorSetLayout frameLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet frameSet = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline modelPipeline = VK_NULL_HANDLE;
    VkPipeline skyPipeline = VK_NULL_HANDLE;
    VkPipeline spritePipeline = VK_NULL_HANDLE;
    VkPipeline shadowPipeline = VK_NULL_HANDLE;
    VkPipeline cloudPipeline = VK_NULL_HANDLE;
    VkPipeline waterPipeline = VK_NULL_HANDLE;
    VkPipeline effectPipeline = VK_NULL_HANDLE;
    std::vector<EffectVertex> effects;
    Buffer effectBuffer; // grown as needed
    VkSampler depthSampler = VK_NULL_HANDLE;
    Image shadowMap; // sized by ShadowSettings::resolution, made once
    VkSampler shadowSampler = VK_NULL_HANDLE;
    Buffer frameBuffer;  // FrameConstants
    Buffer waterBuffer;  // shared surface optics, lighting and screen constants
    Buffer terrainLightBuffer;
    std::vector<terrain_lights::ShaderLights> terrainLightRows;
    std::vector<uint32_t> meshLightRows, terrainLightMeshes;
    Buffer spriteBuffer; // sun (instance 0), moon (1), stars, as vec4 (point, half-size)
    uint32_t starCount = 0;

    // Meshes, by id - 1. A slot with no index buffer is free (or an empty mesh).
    struct Mesh {
        Buffer vertices, indices;
        uint32_t indexCount = 0;
        glm::vec3 centre{0}; // bounding sphere in model space
        float radius = 0;
        bool used = false;
    };
    std::vector<Mesh> meshes;
    std::vector<MeshId> freeIds;
    struct Upload {
        Buffer staging, target;
    };
    std::vector<Upload> pendingUploads; // recorded at the start of the next frame
    std::vector<Buffer> retired;        // freed once the frame that last used them is done
    uint32_t lastDrawn = 0, lastShadowDrawn = 0;

    // Off-screen targets (sized with the swap chain).
    Image sceneColour;
    Image waterColour, waterDepth;
    bool waterSnapshotReady = false;
    Image depthImages[2], history[2];
    Image smaaEdges, smaaWeights;
    int parity = 0;
    bool postFresh = true;    // the targets were just made: last frame's images hold nothing
    bool historyValid = false;
    uint32_t aaFrame = 0;
    glm::vec2 previousJitter{0};
    glm::mat4 previousViewProjection{1};

    // Post pass.
    VkSampler linearSampler = VK_NULL_HANDLE, pointSampler = VK_NULL_HANDLE;
    Image areaTex, searchTex;
    VkDescriptorSetLayout postSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout postLayout = VK_NULL_HANDLE;
    VkDescriptorPool postPool = VK_NULL_HANDLE;
    VkDescriptorSet taaSets[2]{}, copySets[2]{}, edgeSets[2]{}, blendSets[2]{}, weightSet = VK_NULL_HANDLE;
    VkPipeline taaPipeline = VK_NULL_HANDLE, copyPipeline = VK_NULL_HANDLE;
    VkPipeline smaaEdgePipeline = VK_NULL_HANDLE, smaaWeightPipeline = VK_NULL_HANDLE, smaaBlendPipeline = VK_NULL_HANDLE;

    // Interface pass.
    VkPipeline uiPipeline = VK_NULL_HANDLE;
    VkPipeline minimapPipeline = VK_NULL_HANDLE; // needs a depth part in the interface's stencil image
    VkPipeline minimapOutline = VK_NULL_HANDLE;  // the same without depth writes
    VkPipeline uiItemPipeline = VK_NULL_HANDLE;  // item models in the interface (screens §8.1.6)
    std::vector<DrawItem> minimapItems;
    VkPipeline uiMaskIn = VK_NULL_HANDLE, uiMaskOut = VK_NULL_HANDLE; // stencil only (masks)
    VkFormat uiStencilFormat = VK_FORMAT_UNDEFINED;
    Image uiStencil;
    VkExtent2D uiStencilSize{};
    std::vector<ui::Batch> uiBatches;
    std::vector<ui::ItemDraw> uiItems;
    VkDescriptorSet uiSet = VK_NULL_HANDLE;
    Image uiAtlas;
    Buffer uiVertices;
    std::vector<UiVertex> uiTriangles;
    VkExtent2D uiAtlasSize{};
    std::vector<uint8_t> uiAtlasPixels;
    bool uiAtlasDirty = false;
};
