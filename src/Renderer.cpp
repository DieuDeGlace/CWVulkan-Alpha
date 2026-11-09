#include "Renderer.h"
#include "Config.h"
#include "Install.h"
#include "VulkanSync.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <stb_image_write.h>

#include "model_vert.h"
#include "model_frag.h"
#include "sky_vert.h"
#include "sky_frag.h"
#include "sprite_vert.h"
#include "sprite_frag.h"
#include "minimap_vert.h"
#include "minimap_frag.h"
#include "item_vert.h"
#include "shadow_vert.h"
#include "cloud_vert.h"
#include "cloud_frag.h"
#include "water_frag.h"
#include "effect_vert.h"
#include "effect_frag.h"

using vksync::apply;
using vksync::barrier;

namespace {
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
} // namespace

void AntiAliasing::load(const Config &c) {
    auto percent = [&](const char *key, float value, float most) {
        return std::clamp(c.number("AntiAliasing", key, (int)(value * 100 + .5f)) / 100.f, 0.f, most);
    };
    taa = c.number("AntiAliasing", "TAA", taa) != 0;
    smaa = c.number("AntiAliasing", "SMAA", smaa) != 0;
    historyWeight = percent("HistoryPercent", historyWeight, .85f);
    edgeHistoryWeight = percent("EdgeHistoryPercent", edgeHistoryWeight, .95f);
    resolveBlur = percent("BlurPercent", resolveBlur, .5f);
    jitterScale = percent("JitterPercent", jitterScale, 1);
}

// What differs between the scene's pipelines.
struct Renderer::PipelineDesc {
    const uint32_t *vertexCode;
    size_t vertexBytes;
    const uint32_t *fragmentCode; // null for a depth-only pass
    size_t fragmentBytes;
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    bool depth = true;  // test and write depth
    bool blend = false; // source-alpha / inverse-source-alpha
    bool cull = true;
    bool depthOnly = false; // no colour target (the shadow pass)
    bool depthWrite = true; // (false: test against depth without writing it)
    float depthBias = 0;    // slope-scaled depth bias, in units of the depth slope
    // Another depth-stencil target than the scene's (the interface's, for the corner map).
    VkFormat depthFormat = VK_FORMAT_UNDEFINED, stencilFormat = VK_FORMAT_UNDEFINED;
};

bool Renderer::init(GLFWwindow *w, bool wantVsync, const AntiAliasing &settings, const ShadowSettings &shadows,
                    std::string *error) {
    window = w;
    vsync = wantVsync;
    aa = settings;
    shadowSettings = shadows;
    if (volkInitialize() != VK_SUCCESS) {
        *error = "Vulkan is not installed (vulkan-1.dll not found)";
        return false;
    }
    if (!createDevice(error))
        return false;
    if (!createScenePipelines() || !createPostResources() || !createUiResources() || !createMinimapPipeline()) {
        *error = "could not create the pipelines";
        return false;
    }
    if (!createSwapchain()) {
        *error = "could not create the swap chain";
        return false;
    }
    return true;
}

bool Renderer::createDevice(std::string *error) {
    uint32_t extensionCount = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "AlphaVulkan";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = extensionCount;
    info.ppEnabledExtensionNames = extensions;
    if (vkCreateInstance(&info, nullptr, &instance) != VK_SUCCESS) {
        *error = "vkCreateInstance failed (Vulkan 1.3 is needed)";
        return false;
    }
    volkLoadInstance(instance);
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        *error = "cannot create a window surface";
        return false;
    }

    // The first Vulkan 1.3 GPU with a queue that draws and presents; discrete GPUs first.
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> gpus(count);
    vkEnumeratePhysicalDevices(instance, &count, gpus.data());
    std::stable_sort(gpus.begin(), gpus.end(), [](VkPhysicalDevice a, VkPhysicalDevice b) {
        VkPhysicalDeviceProperties pa, pb;
        vkGetPhysicalDeviceProperties(a, &pa);
        vkGetPhysicalDeviceProperties(b, &pb);
        return (pa.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) >
               (pb.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    });
    for (VkPhysicalDevice candidate : gpus) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(candidate, &props);
        if (props.apiVersion < VK_API_VERSION_1_3)
            continue;
        uint32_t families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &families, nullptr);
        std::vector<VkQueueFamilyProperties> familyProps(families);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &families, familyProps.data());
        for (uint32_t i = 0; i < families && !gpu; i++) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &present);
            if ((familyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                gpu = candidate;
                queueFamily = i;
            }
        }
        if (gpu)
            break;
    }
    if (!gpu) {
        *error = "no Vulkan 1.3 GPU can draw to this window";
        return false;
    }

    const float priority = 1;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;
    const char *swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, &features13};
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = &swapchainExtension;
    if (vkCreateDevice(gpu, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
        *error = "vkCreateDevice failed";
        return false;
    }
    volkLoadDevice(device);
    vkGetDeviceQueue(device, queueFamily, 0, &queue);

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamily;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &allocInfo, &commands);
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(device, &semInfo, nullptr, &imageReady);
    vkCreateSemaphore(device, &semInfo, nullptr, &renderDone);
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device, &fenceInfo, nullptr, &frameDone);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, formats.data());
    colourFormat = formats.empty() ? VK_FORMAT_B8G8R8A8_UNORM : formats[0].format;
    for (const auto &f : formats)
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM)
            colourFormat = f.format;
    return true;
}

Renderer::Image Renderer::makeImage(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage,
                                    VkImageAspectFlags aspect) {
    Image img;
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = {w, h, 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.usage = usage;
    if (vkCreateImage(device, &info, nullptr, &img.image) != VK_SUCCESS)
        return {};
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, img.image, &req);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &alloc, nullptr, &img.memory);
    vkBindImageMemory(device, img.image, img.memory, 0);
    VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    v.image = img.image;
    v.viewType = VK_IMAGE_VIEW_TYPE_2D;
    v.format = format;
    v.subresourceRange = {aspect, 0, 1, 0, 1};
    vkCreateImageView(device, &v, nullptr, &img.view);
    return img;
}

void Renderer::freeImage(Image &img) {
    vkDestroyImageView(device, img.view, nullptr);
    vkDestroyImage(device, img.image, nullptr);
    vkFreeMemory(device, img.memory, nullptr);
    img = {};
}

bool Renderer::createSwapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps);
    extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        extent = {(uint32_t)w, (uint32_t)h};
    }
    if (extent.width == 0 || extent.height == 0)
        return true; // minimised: nothing to draw into until the window comes back

    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = surface;
    info.minImageCount = std::max(caps.minImageCount, 2u);
    if (caps.maxImageCount)
        info.minImageCount = std::min(info.minImageCount, caps.maxImageCount);
    info.imageFormat = colourFormat;
    info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    // Transfer source as well, so a finished frame can be copied out for --capture.
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!vsync) {
        uint32_t modeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &modeCount, nullptr);
        std::vector<VkPresentModeKHR> modes(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &modeCount, modes.data());
        for (VkPresentModeKHR mode : modes)
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR || mode == VK_PRESENT_MODE_MAILBOX_KHR)
                info.presentMode = mode;
    }
    info.clipped = VK_TRUE;
    if (vkCreateSwapchainKHR(device, &info, nullptr, &swapchain) != VK_SUCCESS)
        return false;

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
    swapImages.resize(count);
    vkGetSwapchainImagesKHR(device, swapchain, &count, swapImages.data());
    for (VkImage image : swapImages) {
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        v.image = image;
        v.viewType = VK_IMAGE_VIEW_TYPE_2D;
        v.format = colourFormat;
        v.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkImageView view;
        vkCreateImageView(device, &v, nullptr, &view);
        swapViews.push_back(view);
    }
    return createPostTargets();
}

void Renderer::destroySwapchain() {
    destroyPostTargets();
    for (VkImageView view : swapViews)
        vkDestroyImageView(device, view, nullptr);
    swapViews.clear();
    swapImages.clear();
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
}

bool Renderer::recreateSwapchain() {
    vkDeviceWaitIdle(device);
    destroySwapchain();
    return createSwapchain();
}

VkPipeline Renderer::makePipeline(const PipelineDesc &desc) {
    auto module = [&](const uint32_t *code, size_t bytes) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = bytes;
        info.pCode = code;
        VkShaderModule m = VK_NULL_HANDLE;
        vkCreateShaderModule(device, &info, nullptr, &m);
        return m;
    };
    VkShaderModule vert = module(desc.vertexCode, desc.vertexBytes);
    VkShaderModule frag = desc.fragmentCode ? module(desc.fragmentCode, desc.fragmentBytes) : VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    input.vertexBindingDescriptionCount = (uint32_t)desc.bindings.size();
    input.pVertexBindingDescriptions = desc.bindings.data();
    input.vertexAttributeDescriptionCount = (uint32_t)desc.attributes.size();
    input.pVertexAttributeDescriptions = desc.attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = desc.cull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    // Faces wind counter-clockwise seen from outside. Vulkan's framebuffer y points down, which
    // mirrors the winding, and the projection's y flip mirrors it back.
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1;
    if (desc.depthBias > 0) {
        raster.depthBiasEnable = VK_TRUE;
        raster.depthBiasSlopeFactor = desc.depthBias;
    }
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = desc.depth;
    depth.depthWriteEnable = desc.depth && desc.depthWrite;
    depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // §1.3 step 1
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = 0xf;
    if (desc.blend) {
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = desc.depthOnly ? 0 : 1;
    blend.pAttachments = &blendAttachment;
    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamicStates;
    VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering.colorAttachmentCount = desc.depthOnly ? 0 : 1;
    rendering.pColorAttachmentFormats = &colourFormat;
    rendering.depthAttachmentFormat = desc.depthFormat != VK_FORMAT_UNDEFINED ? desc.depthFormat : DEPTH_FORMAT;
    rendering.stencilAttachmentFormat = desc.stencilFormat;

    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, &rendering};
    info.stageCount = frag ? 2 : 1;
    info.pStages = stages;
    info.pVertexInputState = &input;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = layout;
    VkPipeline pipeline = VK_NULL_HANDLE;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);
    vkDestroyShaderModule(device, vert, nullptr);
    if (frag)
        vkDestroyShaderModule(device, frag, nullptr);
    return pipeline;
}

bool Renderer::createScenePipelines() {
    // The frame constants, seen by every stage, and the shadow map; the model's world matrix is
    // a push constant.
    // Water samples separate opaque color/depth snapshots while writing its
    // nearest surface into the scene depth.
    VkDescriptorSetLayoutBinding bindings[6] = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}};
    VkDescriptorSetLayoutCreateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    setInfo.bindingCount = 6;
    setInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(device, &setInfo, nullptr, &frameLayout);
    VkDescriptorPoolSize poolSizes[3] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &frameLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &frameSet);

    frameBuffer = makeBuffer(sizeof(FrameConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    waterBuffer = makeBuffer(6 * sizeof(glm::vec4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    terrainLightBuffer = makeBuffer(sizeof(terrain_lights::ShaderLights), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    if (!frameBuffer.mapped || !waterBuffer.mapped || !terrainLightBuffer.mapped)
        return false;
    shadowMap = makeImage(shadowSettings.resolution, shadowSettings.resolution, DEPTH_FORMAT,
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          VK_IMAGE_ASPECT_DEPTH_BIT);
    // Comparison sampling with bilinear filtering: each lookup is already a 2 x 2 soft test.
    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW =
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // outside the map: lit
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    vkCreateSampler(device, &samplerInfo, nullptr, &shadowSampler);
    if (!shadowMap.image)
        return false;

    VkDescriptorBufferInfo bufferInfo{frameBuffer.buffer, 0, sizeof(FrameConstants)};
    VkDescriptorImageInfo shadowInfo{shadowSampler, shadowMap.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorBufferInfo terrainInfo{terrainLightBuffer.buffer, 0, terrainLightBuffer.size};
    VkWriteDescriptorSet writes[3]{};
    writes[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writes[0].dstSet = frameSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &bufferInfo;
    writes[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writes[1].dstSet = frameSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &shadowInfo;
    writes[2] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writes[2].dstSet = frameSet;
    writes[2].dstBinding = 5;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &terrainInfo;
    vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);

    // Slot 0 is the sun and slot 1 the moon, rewritten every frame; the stars never change.
    const std::vector<alpha::Star> stars = alpha::stars();
    starCount = (uint32_t)stars.size();
    spriteBuffer = makeBuffer(sizeof(glm::vec4) * (2 + stars.size()), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    auto *sprites = (glm::vec4 *)spriteBuffer.mapped;
    for (size_t i = 0; i < stars.size(); i++)
        sprites[2 + i] = glm::vec4(stars[i].position, stars[i].size);

    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) + 2 * sizeof(glm::vec4)};
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &frameLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &push;
    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout);

    PipelineDesc model{spv_model_vert, sizeof(spv_model_vert), spv_model_frag, sizeof(spv_model_frag)};
    model.bindings = {{0, sizeof(ModelVertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    model.attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, normal)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ModelVertex, colour)},
        {3, 0, VK_FORMAT_R32_SFLOAT, offsetof(ModelVertex, sway)},
    };
    modelPipeline = makePipeline(model);

    PipelineDesc sky{spv_sky_vert, sizeof(spv_sky_vert), spv_sky_frag, sizeof(spv_sky_frag)};
    sky.depth = false;
    sky.cull = false;
    skyPipeline = makePipeline(sky);

    PipelineDesc sprite{spv_sprite_vert, sizeof(spv_sprite_vert), spv_sprite_frag, sizeof(spv_sprite_frag)};
    sprite.bindings = {{0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_INSTANCE}};
    sprite.attributes = {{0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0}};
    sprite.depth = false;
    sprite.blend = true;
    sprite.cull = false;
    spritePipeline = makePipeline(sprite);

    PipelineDesc shadow{spv_shadow_vert, sizeof(spv_shadow_vert), nullptr, 0};
    shadow.bindings = model.bindings;
    shadow.attributes = {model.attributes[0], model.attributes[3]};
    shadow.cull = false;
    shadow.depthOnly = true;
    shadow.depthBias = 1.5f;
    shadowPipeline = makePipeline(shadow);

    PipelineDesc cloud = model;
    cloud.vertexCode = spv_cloud_vert;
    cloud.vertexBytes = sizeof(spv_cloud_vert);
    cloud.fragmentCode = spv_cloud_frag;
    cloud.fragmentBytes = sizeof(spv_cloud_frag);
    cloudPipeline = makePipeline(cloud);

    PipelineDesc water = model;
    water.fragmentCode = spv_water_frag;
    water.fragmentBytes = sizeof(spv_water_frag);
    water.blend = true;
    water.cull = false;       // seen from above and below
    water.depthWrite = true;  // the shader samples a separate opaque snapshot
    waterPipeline = makePipeline(water);

    PipelineDesc effect{spv_effect_vert, sizeof(spv_effect_vert), spv_effect_frag, sizeof(spv_effect_frag)};
    effect.bindings = {{0, sizeof(EffectVertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    effect.attributes = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EffectVertex, position)},
                         {1, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(EffectVertex, rgba)}};
    effect.blend = true;
    effect.cull = false;
    effect.depthWrite = false;
    effectPipeline = makePipeline(effect);

    VkSamplerCreateInfo depthInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    depthInfo.magFilter = depthInfo.minFilter = VK_FILTER_NEAREST;
    depthInfo.addressModeU = depthInfo.addressModeV = depthInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(device, &depthInfo, nullptr, &depthSampler);
    return modelPipeline && skyPipeline && spritePipeline && shadowPipeline && cloudPipeline && waterPipeline &&
           effectPipeline;
}

uint32_t Renderer::memoryType(uint32_t bits, VkMemoryPropertyFlags flags) const {
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(gpu, &props);
    for (uint32_t i = 0; i < props.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    return 0;
}

// A host-visible buffer, mapped for its whole life.
Renderer::Buffer Renderer::makeBuffer(VkDeviceSize size, VkBufferUsageFlags usage) {
    Buffer b;
    b.size = size;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage;
    vkCreateBuffer(device, &info, nullptr, &b.buffer);
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, b.buffer, &req);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memoryType(req.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &alloc, nullptr, &b.memory);
    vkBindBufferMemory(device, b.buffer, b.memory, 0);
    vkMapMemory(device, b.memory, 0, size, 0, &b.mapped);
    return b;
}

void Renderer::freeBuffer(Buffer &b) {
    vkDestroyBuffer(device, b.buffer, nullptr);
    vkFreeMemory(device, b.memory, nullptr); // (also unmaps)
    b = {};
}

// A buffer in GPU memory, filled by a copy from a staging buffer.
Renderer::Buffer Renderer::makeDeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage) {
    Buffer b;
    b.size = size;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(device, &info, nullptr, &b.buffer);
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, b.buffer, &req);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &alloc, nullptr, &b.memory);
    vkBindBufferMemory(device, b.buffer, b.memory, 0);
    return b;
}

MeshId Renderer::addMesh(const ModelMesh &mesh) {
    MeshId id;
    if (!freeIds.empty()) {
        id = freeIds.back();
        freeIds.pop_back();
    } else {
        meshes.emplace_back();
        id = (MeshId)meshes.size();
    }
    Mesh &m = meshes[id - 1];
    m = {};
    m.used = true;
    m.indexCount = (uint32_t)mesh.indices.size();
    if (!m.indexCount)
        return id;

    // Bounding sphere: the middle of the box round the vertices, and the farthest vertex.
    glm::vec3 lo(1e30f), hi(-1e30f);
    for (const ModelVertex &v : mesh.vertices) {
        lo = glm::min(lo, v.position);
        hi = glm::max(hi, v.position);
    }
    m.centre = (lo + hi) * .5f;
    for (const ModelVertex &v : mesh.vertices)
        m.radius = std::max(m.radius, glm::length(v.position - m.centre));

    auto upload = [&](const void *data, size_t bytes, VkBufferUsageFlags usage) {
        Upload u{makeBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT), makeDeviceBuffer(bytes, usage)};
        std::memcpy(u.staging.mapped, data, bytes);
        pendingUploads.push_back(u);
        return u.target;
    };
    m.vertices = upload(mesh.vertices.data(), mesh.vertices.size() * sizeof(ModelVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m.indices = upload(mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    return id;
}

void Renderer::removeMesh(MeshId id) {
    if (id == 0 || id > meshes.size() || !meshes[id - 1].used)
        return;
    Mesh &m = meshes[id - 1];
    // A copy not yet recorded is dropped with its staging buffer.
    for (auto u = pendingUploads.begin(); u != pendingUploads.end();)
        if (u->target.buffer == m.vertices.buffer || u->target.buffer == m.indices.buffer) {
            freeBuffer(u->staging);
            u = pendingUploads.erase(u);
        } else
            ++u;
    // The frame in flight may still read them: free after its fence (freeRetired).
    if (m.vertices.buffer)
        retired.push_back(m.vertices);
    if (m.indices.buffer)
        retired.push_back(m.indices);
    m = {};
    freeIds.push_back(id);
}

void Renderer::freeRetired() {
    for (Buffer &b : retired)
        freeBuffer(b);
    retired.clear();
}

// Copies the meshes added since the last frame into GPU memory, before anything draws.
void Renderer::recordUploads(VkCommandBuffer cmd) {
    if (pendingUploads.empty())
        return;
    for (const Upload &u : pendingUploads) {
        VkBufferCopy region{0, 0, u.staging.size};
        vkCmdCopyBuffer(cmd, u.staging.buffer, u.target.buffer, 1, &region);
        retired.push_back(u.staging); // done with once this frame has run
    }
    pendingUploads.clear();
    VkMemoryBarrier2 copied{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    copied.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    copied.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    copied.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
    copied.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT;
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.memoryBarrierCount = 1;
    dependency.pMemoryBarriers = &copied;
    vkCmdPipelineBarrier2(cmd, &dependency);
}

namespace {

// The six planes of a clip matrix's view volume (Vulkan depth 0..1), each as (normal, d) with
// the inside where dot(normal, p) + d >= 0.
std::array<glm::vec4, 6> planesOf(const glm::mat4 &clip) {
    const glm::mat4 t = glm::transpose(clip);
    return {t[3] + t[0], t[3] - t[0], t[3] + t[1], t[3] - t[1], t[2], t[3] - t[2]};
}

bool sphereInside(const std::array<glm::vec4, 6> &planes, glm::vec3 centre, float radius) {
    for (const glm::vec4 &p : planes)
        if (glm::dot(glm::vec3(p), centre) + p.w < -radius * glm::length(glm::vec3(p)))
            return false;
    return true;
}

} // namespace

// Screens §5.1: the world's vertex layout into the swap image, over the interface's
// depth-stencil image (only when that has depth).
bool Renderer::createMinimapPipeline() {
    if (uiStencilFormat == VK_FORMAT_S8_UINT)
        return true; // no corner map on this device
    PipelineDesc map{spv_minimap_vert, sizeof(spv_minimap_vert), spv_minimap_frag, sizeof(spv_minimap_frag)};
    map.bindings = {{0, sizeof(ModelVertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    map.attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, normal)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ModelVertex, colour)},
        {3, 0, VK_FORMAT_R32_SFLOAT, offsetof(ModelVertex, sway)},
    };
    map.depthFormat = map.stencilFormat = uiStencilFormat;
    minimapPipeline = makePipeline(map);
    map.depthWrite = false;
    minimapOutline = makePipeline(map);
    // Screens §8.1.6: item models in the interface, lit in camera space. Their image may be
    // mirrored, so both windings are drawn.
    PipelineDesc item = map;
    item.vertexCode = spv_item_vert;
    item.vertexBytes = sizeof(spv_item_vert);
    item.depthWrite = true;
    item.cull = false;
    uiItemPipeline = makePipeline(item);
    return minimapPipeline != VK_NULL_HANDLE && minimapOutline != VK_NULL_HANDLE && uiItemPipeline != VK_NULL_HANDLE;
}

void Renderer::recordMinimap(VkCommandBuffer cmd, uint32_t imageIndex, const FrameConstants &frame) {
    if (!minimapPipeline || minimapItems.empty())
        return;
    VkRenderingAttachmentInfo colour{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colour.imageView = swapViews[imageIndex];
    colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colour.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depth.imageView = uiStencil.view;
    depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.clearValue.depthStencil = {1, 0};
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    const VkRect2D sc{{0, 0}, extent};
    ri.renderArea = sc;
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &colour;
    ri.pDepthAttachment = &depth;
    ri.pStencilAttachment = &depth;
    VkImageMemoryBarrier2 barriers[2] = {
        barrier(swapImages[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
        barrier(uiStencil.image, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)};
    apply(cmd, barriers, 2);
    vkCmdBeginRendering(cmd, &ri);
    const VkViewport vp{0, 0, (float)extent.width, (float)extent.height, 0, 1};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &frameSet, 0, nullptr);
    // In list order: tiles, border markers, then each creature's outline and coloured passes.
    VkPipeline bound = VK_NULL_HANDLE;
    for (const DrawItem &item : minimapItems) {
        if (item.mesh == 0 || item.mesh > meshes.size() || !meshes[item.mesh - 1].indexCount)
            continue;
        const VkPipeline pipeline = item.shading == Shading::MapOutline ? minimapOutline : minimapPipeline;
        if (pipeline != bound)
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bound = pipeline);
        struct {
            glm::mat4 world;
            glm::vec4 material;
        } constants{item.world, item.material};
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof constants, &constants);
        const Mesh &m = meshes[item.mesh - 1];
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m.vertices.buffer, &offset);
        vkCmdBindIndexBuffer(cmd, m.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, m.indexCount, 1, 0, 0, 0);
    }
    vkCmdEndRendering(cmd);
}

uint32_t Renderer::drawItems(VkCommandBuffer cmd, const std::vector<DrawItem> &items, const glm::mat4 &clip,
                             bool shadowPass, Shading only) {
    const auto planes = planesOf(clip);
    uint32_t drawn = 0;
    for (const DrawItem &item : items) {
        if (item.mesh == 0 || item.mesh > meshes.size() || (shadowPass ? !item.castsShadow : item.shading != only))
            continue;
        const Mesh &m = meshes[item.mesh - 1];
        if (!m.indexCount)
            continue;
        const glm::vec3 centre = glm::vec3(item.world * glm::vec4(m.centre, 1));
        const float scale = std::max({glm::length(glm::vec3(item.world[0])), glm::length(glm::vec3(item.world[1])),
                                      glm::length(glm::vec3(item.world[2]))});
        if (!sphereInside(planes, centre, m.radius * scale))
            continue;
        struct {
            glm::mat4 world;
            glm::vec4 material;
            glm::vec4 extra; // x: shininess, y: terrain light row (zero selects the frame fallback)
        } constants{item.world, item.shading == Shading::World ? item.material : glm::vec4(1),
                    glm::vec4(item.shine && item.shading == Shading::World ? 1.f : 0.f,
                              item.shading == Shading::World && item.mesh < meshLightRows.size()
                                  ? float(meshLightRows[item.mesh]) : 0.f, 0, 0)};
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof constants, &constants);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m.vertices.buffer, &offset);
        vkCmdBindIndexBuffer(cmd, m.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, m.indexCount, 1, 0, 0, 0);
        drawn++;
    }
    return drawn;
}

void Renderer::recordShadow(VkCommandBuffer cmd, const FrameConstants &frame, const std::vector<DrawItem> &items) {
    auto toWrite = barrier(shadowMap.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 0,
                           VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                           VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
    apply(cmd, &toWrite, 1);
    VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depth.imageView = shadowMap.view;
    depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth.clearValue.depthStencil = {1, 0};
    const VkExtent2D size{shadowSettings.resolution, shadowSettings.resolution};
    VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering.renderArea.extent = size;
    rendering.layerCount = 1;
    rendering.pDepthAttachment = &depth;
    vkCmdBeginRendering(cmd, &rendering);
    // With shadows off the map is only cleared (every lookup is then lit).
    lastShadowDrawn = 0;
    if (frame.shadow.x > 0) {
        VkViewport vp{0, 0, (float)size.width, (float)size.height, 0, 1};
        VkRect2D scissor{{0, 0}, size};
        vkCmdSetViewport(cmd, 0, 1, &vp);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &frameSet, 0, nullptr);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
        lastShadowDrawn = drawItems(cmd, items, frame.lightViewProjection, true);
    }
    vkCmdEndRendering(cmd);
    auto toRead = barrier(shadowMap.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                          VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                          VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    apply(cmd, &toRead, 1);
}

void Renderer::recordScene(VkCommandBuffer cmd, const FrameConstants &frame, bool stars, const std::vector<DrawItem> &items) {
    VkImageMemoryBarrier2 targets[2] = {
        barrier(sceneColour.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 0,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
        barrier(depthImages[parity].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 0,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT)};
    apply(cmd, targets, 2);

    VkRenderingAttachmentInfo colour{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colour.imageView = sceneColour.view;
    colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colour.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // the sky covers every pixel
    colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depth.imageView = depthImages[parity].view;
    depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // TAA reads it, now and next frame
    depth.clearValue.depthStencil = {1, 0};
    VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering.renderArea.extent = extent;
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &colour;
    rendering.pDepthAttachment = &depth;
    vkCmdBeginRendering(cmd, &rendering);

    VkViewport vp{0, 0, (float)extent.width, (float)extent.height, 0, 1};
    VkRect2D scissor{{0, 0}, extent};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &frameSet, 0, nullptr);

    // §1.3: the sky gradient, the stars at night, the sun sprite, then the world.
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &spriteBuffer.buffer, &offset);
    if (stars)
        vkCmdDraw(cmd, 6, starCount, 0, 2);
    vkCmdDraw(cmd, 6, 1, 0, 0);
    if (frame.moonSprite.w > 0)
        vkCmdDraw(cmd, 6, 1, 0, 1); // AlphaVulkan's moon, over the stars
    // §1.3 steps 11-12: clouds, then the world.
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cloudPipeline);
    lastDrawn = drawItems(cmd, items, frame.viewProjection, false, Shading::Cloud);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, modelPipeline);
    lastDrawn += drawItems(cmd, items, frame.viewProjection, false, Shading::World);
    // Trails §1.3: the effect triangles, after the solid world and before the water.
    if (!effects.empty()) {
        const VkDeviceSize bytes = effects.size() * sizeof(EffectVertex);
        if (bytes > effectBuffer.size) {
            freeBuffer(effectBuffer); // one frame in flight: the GPU is done with it
            effectBuffer = makeBuffer(bytes * 2, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        }
        std::memcpy(effectBuffer.mapped, effects.data(), bytes);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, effectPipeline);
        vkCmdBindVertexBuffers(cmd, 0, 1, &effectBuffer.buffer, &offset);
        vkCmdDraw(cmd, (uint32_t)effects.size(), 1, 0, 0);
    }
    vkCmdEndRendering(cmd);

    // Snapshot opaque color and depth before drawing water. Refraction includes
    // that background once, and surface depth rejects overlapping farther water.
    const bool anyWater =
        std::any_of(items.begin(), items.end(), [](const DrawItem &i) { return i.shading == Shading::Water; });
    if (!anyWater)
        return;
    snapshotWaterBackground(cmd);
    colour.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    vkCmdBeginRendering(cmd, &rendering);
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &frameSet, 0, nullptr);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, waterPipeline);
    lastDrawn += drawItems(cmd, items, frame.viewProjection, false, Shading::Water);
    vkCmdEndRendering(cmd);
}

void Renderer::setTerrainLights(std::span<const terrain_lights::Selection> selections) {
    meshLightRows.resize(meshes.size() + 1, 0);
    for (uint32_t mesh : terrainLightMeshes)
        if (mesh < meshLightRows.size())
            meshLightRows[mesh] = 0;
    terrainLightMeshes.clear();
    terrainLightMeshes.reserve(selections.size());
    terrainLightRows.resize(selections.size());
    for (size_t i = 0; i < selections.size(); ++i) {
        terrainLightRows[i] = terrain_lights::shaderPrefix(selections[i]);
        const uint32_t mesh = selections[i].mesh;
        if (mesh && mesh < meshLightRows.size()) {
            meshLightRows[mesh] = uint32_t(i + 1);
            terrainLightMeshes.push_back(mesh);
        }
    }
}

bool Renderer::prepareTerrainLights() {
    const VkDeviceSize bytes = std::max<size_t>(terrainLightRows.size(), 1) * sizeof(terrain_lights::ShaderLights);
    if (terrainLightBuffer.size < bytes) {
        Buffer replacement = makeBuffer(std::max(bytes, terrainLightBuffer.size * 2), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        if (!replacement.mapped) {
            freeBuffer(replacement);
            return false;
        }
        freeBuffer(terrainLightBuffer);
        terrainLightBuffer = replacement;
        VkDescriptorBufferInfo info{terrainLightBuffer.buffer, 0, terrainLightBuffer.size};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = frameSet;
        write.dstBinding = 5;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &info;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
    if (terrainLightRows.empty())
        std::memset(terrainLightBuffer.mapped, 0, sizeof(terrain_lights::ShaderLights));
    else
        std::memcpy(terrainLightBuffer.mapped, terrainLightRows.data(), size_t(bytes));
    return true;
}

bool Renderer::draw(const FrameConstants &frameIn, bool stars, const std::vector<DrawItem> &items,
                    const std::filesystem::path &capture) {
    if (!swapchain && (!createSwapchain() || !swapchain))
        return false; // still minimised
    vkWaitForFences(device, 1, &frameDone, VK_TRUE, UINT64_MAX);
    if (!prepareTerrainLights())
        return false;
    uint32_t index = 0;
    VkResult acquired = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageReady, VK_NULL_HANDLE, &index);
    if (acquired == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return false;
    }
    vkResetFences(device, 1, &frameDone);
    freeRetired(); // the last frame is done with them
    prepareInterface();

    // TAA: an eight-frame Halton sub-pixel jitter; the scene moves by `jitter` pixels and the
    // resolve samples back at the same offset.
    glm::vec2 jitter(0);
    if (aa.taa) {
        auto halton = [](uint32_t i, uint32_t base) {
            float f = 1, r = 0;
            for (; i; i /= base) {
                f /= base;
                r += f * (i % base);
            }
            return r;
        };
        aaFrame++;
        jitter = glm::vec2(halton(aaFrame % 8 + 1, 2) - .5f, halton(aaFrame % 8 + 1, 3) - .5f) * aa.jitterScale;
    }
    glm::mat4 jitterClip(1); // shifts clip x and y by `jitter` pixels (times w)
    jitterClip[3][0] = 2 * jitter.x / extent.width;
    jitterClip[3][1] = 2 * jitter.y / extent.height;
    FrameConstants frame = frameIn;
    frame.viewProjection = jitterClip * frame.viewProjection;
    frame.skyViewProjection = jitterClip * frame.skyViewProjection;
    const glm::mat4 clipToPrevious = previousViewProjection * glm::inverse(frame.viewProjection);

    // One frame is in flight, so its constants and image bindings can now change.
    std::memcpy(frameBuffer.mapped, &frame, sizeof frame);
    updateWaterConstants(frame);
    auto *sprites = (glm::vec4 *)spriteBuffer.mapped;
    sprites[0] = glm::vec4(100.f * glm::vec3(frame.lightDirection), 40.f); // §8.4
    sprites[1] = glm::vec4(100.f * glm::vec3(frame.moonSprite), 9.f);       // the disc is 0.6 of it

    vkResetCommandBuffer(commands, 0);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commands, &begin);
    recordUploads(commands);
    recordShadow(commands, frame, items);
    recordScene(commands, frame, stars, items);
    recordPost(commands, index, jitter, clipToPrevious);
    recordMinimap(commands, index, frame);
    recordInterface(commands, index);
    auto toPresent = barrier(swapImages[index], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, 0);
    apply(commands, &toPresent, 1);
    vkEndCommandBuffer(commands);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageReady;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commands;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &renderDone;
    vkQueueSubmit(queue, 1, &submit, frameDone);

    historyValid = true;
    previousJitter = jitter;
    previousViewProjection = frame.viewProjection;
    parity = 1 - parity;

    bool saved = true;
    if (!capture.empty()) {
        vkWaitForFences(device, 1, &frameDone, VK_TRUE, UINT64_MAX);
        saved = saveImage(swapImages[index], capture);
    }

    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderDone;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain;
    present.pImageIndices = &index;
    VkResult presented = vkQueuePresentKHR(queue, &present);
    if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR)
        recreateSwapchain();
    return saved;
}

bool Renderer::saveImage(VkImage image, const std::filesystem::path &file) {
    const VkDeviceSize bytes = (VkDeviceSize)extent.width * extent.height * 4;
    Buffer staging = makeBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &allocInfo, &cmd);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    auto toCopy = barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0,
                          VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    apply(cmd, &toCopy, 1);
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {extent.width, extent.height, 1};
    vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging.buffer, 1, &region);
    auto back = barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                        VK_PIPELINE_STAGE_2_NONE, 0);
    apply(cmd, &back, 1);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);

    const auto *pixels = (const uint8_t *)staging.mapped;
    std::vector<uint8_t> rgba(pixels, pixels + bytes);
    freeBuffer(staging);
    if (colourFormat == VK_FORMAT_B8G8R8A8_UNORM || colourFormat == VK_FORMAT_B8G8R8A8_SRGB)
        for (size_t i = 0; i < rgba.size(); i += 4)
            std::swap(rgba[i], rgba[i + 2]);
    for (size_t i = 3; i < rgba.size(); i += 4)
        rgba[i] = 255;
    return stbi_write_png(utf8(file).c_str(), (int)extent.width, (int)extent.height, 4, rgba.data(),
                          (int)extent.width * 4) != 0;
}

Renderer::~Renderer() {
    if (!device) {
        if (instance) {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            vkDestroyInstance(instance, nullptr);
        }
        return;
    }
    vkDeviceWaitIdle(device);
    for (MeshId id = 1; id <= meshes.size(); id++)
        removeMesh(id); // (also drops their copies still waiting)
    freeRetired();
    freeBuffer(spriteBuffer);
    freeBuffer(effectBuffer);
    freeBuffer(frameBuffer);
    freeBuffer(waterBuffer);
    freeBuffer(terrainLightBuffer);
    destroySwapchain();
    destroyUiResources();
    destroyPostResources();
    vkDestroySampler(device, depthSampler, nullptr);
    for (VkPipeline p :
         {modelPipeline, skyPipeline, spritePipeline, shadowPipeline, cloudPipeline, waterPipeline, effectPipeline})
        vkDestroyPipeline(device, p, nullptr);
    vkDestroySampler(device, shadowSampler, nullptr);
    freeImage(shadowMap);
    vkDestroyPipelineLayout(device, layout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, frameLayout, nullptr);
    vkDestroyFence(device, frameDone, nullptr);
    vkDestroySemaphore(device, renderDone, nullptr);
    vkDestroySemaphore(device, imageReady, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
