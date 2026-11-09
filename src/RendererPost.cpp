// Anti-aliasing after the scene: TAA (from the user's TemporalAA mod, by way of CubeVulkan)
// then SMAA Ultra, onto the screen.
#include "Renderer.h"
#include "VulkanSync.h"
#include <cstring>

#include "post_vert.h"
#include "copy_frag.h"
#include "taa_frag.h"
#include "smaa_edge_vert.h"
#include "smaa_edge_frag.h"
#include "smaa_weight_vert.h"
#include "smaa_weight_frag.h"
#include "smaa_blend_vert.h"
#include "smaa_blend_frag.h"
#include "../third_party/smaa/AreaTex.h"
#include "../third_party/smaa/SearchTex.h"

using vksync::apply;
using vksync::barrier;

namespace {
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

// Colour image written by a pass, then read by the next.
VkImageMemoryBarrier2 toWrite(VkImage image) {
    // (The source stage includes colour output so a swap image waits for its acquire.)
    return barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                   VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
}
VkImageMemoryBarrier2 toRead(VkImage image) {
    return barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                   VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                   VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
}
} // namespace

Renderer::Image Renderer::uploadImage(uint32_t w, uint32_t h, VkFormat format, const void *data, size_t bytes) {
    Image img = makeImage(w, h, format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT);
    if (!img.image)
        return img;
    Buffer staging = makeBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    std::memcpy(staging.mapped, data, bytes);
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &allocInfo, &cmd);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    auto in = barrier(img.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0, VK_PIPELINE_STAGE_2_COPY_BIT,
                      VK_ACCESS_2_TRANSFER_WRITE_BIT);
    apply(cmd, &in, 1);
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {w, h, 1};
    vkCmdCopyBufferToImage(cmd, staging.buffer, img.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    auto out = barrier(img.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT,
                       VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                       VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    apply(cmd, &out, 1);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    freeBuffer(staging);
    return img;
}

bool Renderer::createPostResources() {
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_LINEAR;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(device, &si, nullptr, &linearSampler);
    si.magFilter = si.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(device, &si, nullptr, &pointSampler);
    // SMAA's precomputed area (RG8) and search (R8) textures.
    areaTex = uploadImage(AREATEX_WIDTH, AREATEX_HEIGHT, VK_FORMAT_R8G8_UNORM, areaTexBytes, AREATEX_SIZE);
    searchTex = uploadImage(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, VK_FORMAT_R8_UNORM, searchTexBytes, SEARCHTEX_SIZE);
    if (!areaTex.image || !searchTex.image)
        return false;

    // One layout for every post pass: up to four images, 128 bytes of constants.
    VkDescriptorSetLayoutBinding bindings[4];
    for (uint32_t i = 0; i < 4; i++)
        bindings[i] = {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT};
    VkDescriptorSetLayoutCreateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    setInfo.bindingCount = 4;
    setInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(device, &setInfo, nullptr, &postSetLayout);
    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 128};
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &postSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &push;
    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &postLayout);
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 * 4};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 10; // (the tenth is the interface atlas, RendererUi.cpp)
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &size;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &postPool);
    VkDescriptorSet *sets[9] = {&taaSets[0], &taaSets[1], &copySets[0], &copySets[1], &edgeSets[0],
                                &edgeSets[1], &weightSet,  &blendSets[0], &blendSets[1]};
    for (VkDescriptorSet *s : sets) {
        VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = postPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &postSetLayout;
        vkAllocateDescriptorSets(device, &allocInfo, s);
    }

    auto module = [&](const uint32_t *code, size_t bytes) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = bytes;
        info.pCode = code;
        VkShaderModule m = VK_NULL_HANDLE;
        vkCreateShaderModule(device, &info, nullptr, &m);
        return m;
    };
    // A full-screen pass: no vertex input, no depth, no blending, one colour target.
    auto pipeline = [&](const uint32_t *vs, size_t vsBytes, const uint32_t *fs, size_t fsBytes, VkFormat format,
                        VkPipeline &out) {
        VkShaderModule v = module(vs, vsBytes), f = module(fs, fsBytes);
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, v, "main"};
        stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, f, "main"};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = viewport.scissorCount = 1;
        VkDynamicState dynamics[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamics;
        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.lineWidth = 1;
        VkPipelineDepthStencilStateCreateInfo depthState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.colorWriteMask = 0xf;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &attachment;
        VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &format;
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, &rendering};
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &input;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewport;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &multisample;
        info.pDepthStencilState = &depthState;
        info.pColorBlendState = &blend;
        info.pDynamicState = &dynamic;
        info.layout = postLayout;
        VkResult r = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &out);
        vkDestroyShaderModule(device, v, nullptr);
        vkDestroyShaderModule(device, f, nullptr);
        return r == VK_SUCCESS;
    };
    return pipeline(spv_post_vert, sizeof spv_post_vert, spv_taa_frag, sizeof spv_taa_frag, colourFormat, taaPipeline) &&
           pipeline(spv_post_vert, sizeof spv_post_vert, spv_copy_frag, sizeof spv_copy_frag, colourFormat, copyPipeline) &&
           pipeline(spv_smaa_edge_vert, sizeof spv_smaa_edge_vert, spv_smaa_edge_frag, sizeof spv_smaa_edge_frag,
                    VK_FORMAT_R8G8_UNORM, smaaEdgePipeline) &&
           pipeline(spv_smaa_weight_vert, sizeof spv_smaa_weight_vert, spv_smaa_weight_frag, sizeof spv_smaa_weight_frag,
                    VK_FORMAT_R8G8B8A8_UNORM, smaaWeightPipeline) &&
           pipeline(spv_smaa_blend_vert, sizeof spv_smaa_blend_vert, spv_smaa_blend_frag, sizeof spv_smaa_blend_frag,
                    colourFormat, smaaBlendPipeline);
}

void Renderer::destroyPostResources() {
    for (VkPipeline p : {taaPipeline, copyPipeline, smaaEdgePipeline, smaaWeightPipeline, smaaBlendPipeline})
        vkDestroyPipeline(device, p, nullptr);
    vkDestroyPipelineLayout(device, postLayout, nullptr);
    vkDestroyDescriptorPool(device, postPool, nullptr);
    vkDestroyDescriptorSetLayout(device, postSetLayout, nullptr);
    vkDestroySampler(device, linearSampler, nullptr);
    vkDestroySampler(device, pointSampler, nullptr);
    freeImage(areaTex);
    freeImage(searchTex);
}

bool Renderer::createPostTargets() {
    const uint32_t w = extent.width, h = extent.height;
    const VkImageUsageFlags colour = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    sceneColour = makeImage(w, h, colourFormat, colour | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    waterColour = makeImage(w, h, colourFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                            VK_IMAGE_ASPECT_COLOR_BIT);
    waterDepth = makeImage(w, h, DEPTH_FORMAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                           VK_IMAGE_ASPECT_DEPTH_BIT);
    waterSnapshotReady = false;
    for (int i = 0; i < 2; i++) {
        depthImages[i] = makeImage(w, h, DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                   VK_IMAGE_ASPECT_DEPTH_BIT);
        history[i] = makeImage(w, h, colourFormat, colour, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    smaaEdges = makeImage(w, h, VK_FORMAT_R8G8_UNORM, colour, VK_IMAGE_ASPECT_COLOR_BIT);
    smaaWeights = makeImage(w, h, VK_FORMAT_R8G8B8A8_UNORM, colour, VK_IMAGE_ASPECT_COLOR_BIT);
    if (!sceneColour.image || !depthImages[0].image || !depthImages[1].image || !history[0].image ||
        !history[1].image || !smaaEdges.image || !smaaWeights.image || !waterColour.image || !waterDepth.image)
        return false;

    auto write = [&](VkDescriptorSet set, std::initializer_list<std::pair<VkImageView, VkSampler>> images) {
        std::vector<VkDescriptorImageInfo> info;
        for (const auto &[view, sampler] : images)
            info.push_back({sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        // Unused bindings point at the first image, so every binding is valid.
        while (info.size() < 4)
            info.push_back(info[0]);
        VkWriteDescriptorSet writes[4]{};
        for (uint32_t i = 0; i < 4; i++) {
            writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writes[i].dstSet = set;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].pImageInfo = &info[i];
        }
        vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);
    };
    for (int p = 0; p < 2; p++) {
        const int q = 1 - p; // last frame's images
        write(taaSets[p], {{sceneColour.view, linearSampler}, {history[q].view, linearSampler},
                           {depthImages[p].view, pointSampler}, {depthImages[q].view, pointSampler}});
        write(copySets[p], {{history[p].view, linearSampler}});
        write(edgeSets[p], {{history[p].view, linearSampler}});
        write(blendSets[p], {{history[p].view, linearSampler}, {smaaWeights.view, linearSampler}});
    }
    write(weightSet, {{smaaEdges.view, linearSampler}, {areaTex.view, linearSampler}, {searchTex.view, pointSampler}});
    postFresh = true;
    historyValid = false;
    return true;
}

void Renderer::destroyPostTargets() {
    for (Image *i : {&sceneColour, &waterColour, &waterDepth, &depthImages[0], &depthImages[1], &history[0], &history[1], &smaaEdges, &smaaWeights})
        freeImage(*i);
    waterSnapshotReady = false;
}

void Renderer::recordPost(VkCommandBuffer cmd, uint32_t imageIndex, glm::vec2 jitter, const glm::mat4 &clipToPrevious) {
    const int p = parity, q = 1 - parity;
    VkViewport vp{0, 0, (float)extent.width, (float)extent.height, 0, 1};
    VkRect2D sc{{0, 0}, extent};
    auto begin = [&](VkImageView view, bool clear) {
        VkRenderingAttachmentInfo colour{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        colour.imageView = view;
        colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colour.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
        ri.renderArea = sc;
        ri.layerCount = 1;
        ri.colorAttachmentCount = 1;
        ri.pColorAttachments = &colour;
        vkCmdBeginRendering(cmd, &ri);
        vkCmdSetViewport(cmd, 0, 1, &vp);
        vkCmdSetScissor(cmd, 0, 1, &sc);
    };
    auto fullscreen = [&](VkPipeline pipeline, VkDescriptorSet set, const void *constants, uint32_t size) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, postLayout, 0, 1, &set, 0, nullptr);
        if (size)
            vkCmdPushConstants(cmd, postLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, size,
                               constants);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    };

    // The scene and its depth become readable. On fresh targets last frame's depth and
    // history are made valid too (and the history is ignored this frame).
    std::vector<VkImageMemoryBarrier2> ready = {
        toRead(sceneColour.image),
        barrier(depthImages[p].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)};
    if (postFresh) {
        ready.push_back(barrier(depthImages[q].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0,
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT));
        ready.push_back(barrier(history[q].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0,
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT));
    }
    ready.push_back(toWrite(history[p].image));
    apply(cmd, ready.data(), (uint32_t)ready.size());

    // TAA resolve into this frame's history (weight 0 = the current image only).
    struct {
        float a[4], b[4];
        float clip[16];
        float previousJitter[4];
    } c{};
    const bool useHistory = aa.taa && historyValid && !postFresh;
    c.a[0] = aa.taa ? aa.historyWeight : 0;
    c.a[1] = useHistory ? 1.f : 0.f;
    c.a[2] = jitter.x;
    c.a[3] = jitter.y;
    c.b[0] = aa.taa ? aa.resolveBlur : 0;
    c.b[1] = aa.edgeHistoryWeight;
    c.b[2] = 1; // reproject with the camera
    c.b[3] = .0005f; // depth tolerance
    std::memcpy(c.clip, &clipToPrevious[0][0], sizeof c.clip);
    c.previousJitter[0] = previousJitter.x;
    c.previousJitter[1] = previousJitter.y;
    begin(history[p].view, false);
    fullscreen(taaPipeline, taaSets[p], &c, sizeof c);
    auto resolved = toRead(history[p].image);
    apply(cmd, &resolved, 1);

    // SMAA Ultra on the resolved image (after it is kept as history), or a plain copy.
    const VkImage swap = swapImages[imageIndex];
    const float metrics[4] = {1.f / extent.width, 1.f / extent.height, (float)extent.width, (float)extent.height};
    if (aa.smaa) {
        auto edgesWrite = toWrite(smaaEdges.image);
        apply(cmd, &edgesWrite, 1);
        begin(smaaEdges.view, true);
        fullscreen(smaaEdgePipeline, edgeSets[p], metrics, sizeof metrics);
        VkImageMemoryBarrier2 next[2] = {toRead(smaaEdges.image), toWrite(smaaWeights.image)};
        apply(cmd, next, 2);
        begin(smaaWeights.view, true);
        fullscreen(smaaWeightPipeline, weightSet, metrics, sizeof metrics);
        VkImageMemoryBarrier2 last[2] = {toRead(smaaWeights.image), toWrite(swap)};
        apply(cmd, last, 2);
        begin(swapViews[imageIndex], false);
        fullscreen(smaaBlendPipeline, blendSets[p], metrics, sizeof metrics);
    } else {
        auto swapWrite = toWrite(swap);
        apply(cmd, &swapWrite, 1);
        begin(swapViews[imageIndex], false);
        fullscreen(copyPipeline, copySets[p], nullptr, 0);
    }
    postFresh = false;
}
