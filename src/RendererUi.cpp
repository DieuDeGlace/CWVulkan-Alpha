// The interface pass: alpha-blended triangles in window pixels over the finished frame
// (alpha-ui-plasma.md §5.1), textured from one atlas.
#include "Renderer.h"
#include "VulkanSync.h"
#include <cstring>

#include "ui_vert.h"
#include "ui_frag.h"

using vksync::apply;
using vksync::barrier;

void Renderer::setInterface(ui::Frame frame) {
    uiTriangles = std::move(frame.triangles);
    uiBatches = std::move(frame.batches);
    uiItems = std::move(frame.items);
}

void Renderer::setInterfaceAtlas(uint32_t width, uint32_t height, std::vector<uint8_t> rgba) {
    uiAtlasSize = {width, height};
    uiAtlasPixels = std::move(rgba);
    uiAtlasDirty = true;
}

bool Renderer::createUiResources() {
    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = postPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &postSetLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &uiSet) != VK_SUCCESS)
        return false;
    // Until the game hands an atlas over, a single white texel.
    setInterfaceAtlas(1, 1, {255, 255, 255, 255});

    auto module = [&](const uint32_t *code, size_t bytes) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = bytes;
        info.pCode = code;
        VkShaderModule m = VK_NULL_HANDLE;
        vkCreateShaderModule(device, &info, nullptr, &m);
        return m;
    };
    // A stencil format for the masks.
    // (Prefer one with depth: the corner map draws over it too, screens §5.1.)
    for (VkFormat format : {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_S8_UINT}) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(gpu, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            uiStencilFormat = format;
            break;
        }
    }
    if (uiStencilFormat == VK_FORMAT_UNDEFINED)
        return false;

    VkShaderModule v = module(spv_ui_vert, sizeof spv_ui_vert), f = module(spv_ui_frag, sizeof spv_ui_frag);
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, v, "main"};
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, f, "main"};
    VkVertexInputBindingDescription binding{0, sizeof(UiVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attributes[3] = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UiVertex, position)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UiVertex, uv)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UiVertex, colour)},
    };
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    input.vertexBindingDescriptionCount = 1;
    input.pVertexBindingDescriptions = &binding;
    input.vertexAttributeDescriptionCount = 3;
    input.pVertexAttributeDescriptions = attributes;
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = viewport.scissorCount = 1;
    VkDynamicState dynamics[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_STENCIL_REFERENCE};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 3;
    dynamic.pDynamicStates = dynamics;
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.lineWidth = 1;
    // Triangles are drawn where the stencil equals the reference: the number of masks around them.
    VkPipelineDepthStencilStateCreateInfo depthState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthState.stencilTestEnable = VK_TRUE;
    depthState.front = {VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL, 0xff, 0xff, 0};
    depthState.back = depthState.front;
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.blendEnable = VK_TRUE;
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    attachment.colorWriteMask = 0xf;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &attachment;
    VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &colourFormat;
    rendering.stencilAttachmentFormat = uiStencilFormat;
    if (uiStencilFormat != VK_FORMAT_S8_UINT)
        rendering.depthAttachmentFormat = uiStencilFormat;
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
    VkResult r = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &uiPipeline);
    // Masks: their shape only counts the stencil up (in) or down (out), and draws no colour.
    attachment.colorWriteMask = 0;
    for (auto [op, pipeline] : {std::pair{VK_STENCIL_OP_INCREMENT_AND_CLAMP, &uiMaskIn},
                                std::pair{VK_STENCIL_OP_DECREMENT_AND_CLAMP, &uiMaskOut}}) {
        depthState.front.passOp = op;
        depthState.back = depthState.front;
        if (r == VK_SUCCESS)
            r = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, pipeline);
    }
    vkDestroyShaderModule(device, v, nullptr);
    vkDestroyShaderModule(device, f, nullptr);
    return r == VK_SUCCESS;
}

void Renderer::destroyUiResources() {
    vkDestroyPipeline(device, uiPipeline, nullptr);
    vkDestroyPipeline(device, uiMaskIn, nullptr);
    vkDestroyPipeline(device, uiMaskOut, nullptr);
    vkDestroyPipeline(device, minimapPipeline, nullptr);
    vkDestroyPipeline(device, uiItemPipeline, nullptr);
    vkDestroyPipeline(device, minimapOutline, nullptr);
    freeImage(uiStencil);
    freeImage(uiAtlas);
    freeBuffer(uiVertices);
}

// Called once the previous frame is finished, so the old atlas and vertices are free.
void Renderer::prepareInterface() {
    if (uiAtlasDirty) {
        freeImage(uiAtlas);
        uiAtlas = uploadImage(uiAtlasSize.width, uiAtlasSize.height, VK_FORMAT_R8G8B8A8_UNORM, uiAtlasPixels.data(),
                              uiAtlasPixels.size());
        VkDescriptorImageInfo image{linearSampler, uiAtlas.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet writes[4]{};
        for (uint32_t i = 0; i < 4; i++) {
            writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writes[i].dstSet = uiSet;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].pImageInfo = &image;
        }
        vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);
        uiAtlasDirty = false;
    }
    if (uiStencilSize.width != extent.width || uiStencilSize.height != extent.height) {
        freeImage(uiStencil);
        const VkImageAspectFlags aspect = uiStencilFormat == VK_FORMAT_S8_UINT
                                              ? VK_IMAGE_ASPECT_STENCIL_BIT
                                              : VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        uiStencil = makeImage(extent.width, extent.height, uiStencilFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                              aspect);
        uiStencilSize = extent;
    }
    const VkDeviceSize bytes = uiTriangles.size() * sizeof(UiVertex);
    if (bytes > uiVertices.size) {
        freeBuffer(uiVertices);
        uiVertices = makeBuffer(bytes * 2, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }
    if (bytes)
        std::memcpy(uiVertices.mapped, uiTriangles.data(), bytes);
}

void Renderer::recordInterface(VkCommandBuffer cmd, uint32_t imageIndex) {
    if (uiTriangles.empty() && uiItems.empty())
        return;
    VkRenderingAttachmentInfo colour{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colour.imageView = swapViews[imageIndex];
    colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colour.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    const VkRect2D sc{{0, 0}, extent};
    ri.renderArea = sc;
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &colour;
    VkRenderingAttachmentInfo stencil{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    stencil.imageView = uiStencil.view;
    stencil.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    stencil.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    stencil.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    stencil.clearValue.depthStencil = {1, 0};
    ri.pStencilAttachment = &stencil;
    if (uiStencilFormat != VK_FORMAT_S8_UINT)
        ri.pDepthAttachment = &stencil;
    // The post pass wrote the swap image; the interface blends over it.
    auto ready = barrier(swapImages[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    const VkImageAspectFlags stencilAspect = uiStencilFormat == VK_FORMAT_S8_UINT
                                                 ? VK_IMAGE_ASPECT_STENCIL_BIT
                                                 : VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    VkImageMemoryBarrier2 barriers[2] = {
        ready, barrier(uiStencil.image, stencilAspect, VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                       VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                       VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)};
    apply(cmd, barriers, 2);
    vkCmdBeginRendering(cmd, &ri);
    const VkViewport vp{0, 0, (float)extent.width, (float)extent.height, 0, 1};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);
    // The triangles' state, bound again after an item model.
    const auto bindTriangles = [&] {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, postLayout, 0, 1, &uiSet, 0, nullptr);
        const float screen[4] = {(float)extent.width, (float)extent.height, 0, 0};
        vkCmdPushConstants(cmd, postLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof screen,
                           screen);
        const VkDeviceSize offset = 0;
        if (uiVertices.buffer)
            vkCmdBindVertexBuffers(cmd, 0, 1, &uiVertices.buffer, &offset);
    };
    bindTriangles();
    VkPipeline bound = VK_NULL_HANDLE;
    for (const ui::Batch &b : uiBatches) {
        if (b.count == 0)
            continue;
        if (b.op == ui::Batch::Item) {
            // Screens §8.1.6: one item model, depth-tested against the others.
            if (!uiItemPipeline || b.first >= uiItems.size())
                continue;
            const ui::ItemDraw &item = uiItems[b.first];
            if (item.mesh == 0 || item.mesh > meshes.size() || !meshes[item.mesh - 1].indexCount)
                continue;
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bound = uiItemPipeline);
            struct {
                glm::mat4 camera;
                glm::vec4 lens;
            } constants{item.camera, item.lens};
            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof constants, &constants);
            const Mesh &m = meshes[item.mesh - 1];
            const VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &m.vertices.buffer, &offset);
            vkCmdBindIndexBuffer(cmd, m.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, m.indexCount, 1, 0, 0, 0);
            continue;
        }
        if (bound == uiItemPipeline && bound != VK_NULL_HANDLE) {
            bindTriangles();
            bound = VK_NULL_HANDLE;
        }
        // In: count up where the parent masks cover; out: count back down; draw: where all cover.
        const VkPipeline pipeline = b.op == ui::Batch::MaskIn ? uiMaskIn : b.op == ui::Batch::MaskOut ? uiMaskOut : uiPipeline;
        if (pipeline != bound)
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bound = pipeline);
        vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, b.op == ui::Batch::MaskOut ? b.level + 1u : b.level);
        vkCmdDraw(cmd, b.count, 1, b.first, 0);
    }
    vkCmdEndRendering(cmd);
}
