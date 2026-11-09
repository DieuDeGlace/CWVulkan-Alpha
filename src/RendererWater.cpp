// The shared surface optics and snapshot pass, adapted from CubeVulkan's
// original GPL RendererWater.cpp. No original-game shader or art is included.
#include "Renderer.h"
#include "VulkanSync.h"
#include <algorithm>
#include <cstring>

namespace {
struct WaterConstants {
    glm::vec4 optics, body, absorption, screen, fog, sky;
};
static_assert(sizeof(WaterConstants) == 96);
}

void Renderer::updateWaterConstants(const FrameConstants &frame) {
    const bool submerged = frame.underwater.w > 0;
    const float brightness = .2f + .8f * std::clamp(frame.material.w, 0.f, 1.f);
    const glm::vec3 fog = submerged ? glm::vec3(frame.underwater) : glm::vec3(frame.fog);
    const auto scattering = submerged ? glm::vec4(fog, 1.f) : glm::vec4(.015f, .27f, .72f, brightness);
    const WaterConstants constants{
        {frame.water.x, frame.water.y, 24.f, .85f}, scattering, {.38f, .23f, .06f, 0},
        {1.f / extent.width, 1.f / extent.height, submerged ? 1.f : 0.f, frame.water.w},
        {fog, frame.params.x > 0 ? 1.f / frame.params.x : 640.f},
        {glm::vec3(.12f, .35f, .62f) * brightness, 1}};
    std::memcpy(waterBuffer.mapped, &constants, sizeof constants);

    const VkDescriptorImageInfo images[] = {
        {depthSampler, waterDepth.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {linearSampler, waterColour.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    const VkDescriptorBufferInfo buffer{waterBuffer.buffer, 0, sizeof constants};
    VkWriteDescriptorSet writes[3]{};
    for (uint32_t index = 0; index < 3; index++) {
        auto &write = writes[index];
        write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = frameSet;
        write.dstBinding = index + 2;
        write.descriptorCount = 1;
        write.descriptorType = index < 2 ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        if (index < 2)
            write.pImageInfo = &images[index];
        else
            write.pBufferInfo = &buffer;
    }
    vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
}

void Renderer::snapshotWaterBackground(VkCommandBuffer cmd) {
    using vksync::apply;
    using vksync::barrier;
    constexpr auto depthStages = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    const auto oldLayout = waterSnapshotReady ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    const auto oldStage = waterSnapshotReady ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_2_NONE;
    const auto oldAccess = waterSnapshotReady ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT : VK_ACCESS_2_NONE;
    const VkImageMemoryBarrier2 before[] = {
        barrier(sceneColour.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT),
        barrier(depthImages[parity].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, depthStages, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT),
        barrier(waterColour.image, VK_IMAGE_ASPECT_COLOR_BIT, oldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                oldStage, oldAccess, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT),
        barrier(waterDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT, oldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                oldStage, oldAccess, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT)};
    apply(cmd, before, 4);
    const auto copy = [&](VkImage source, VkImage destination, VkImageAspectFlags aspect) {
        VkImageCopy region{};
        region.srcSubresource = region.dstSubresource = {aspect, 0, 0, 1};
        region.extent = {extent.width, extent.height, 1};
        vkCmdCopyImage(cmd, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    };
    copy(sceneColour.image, waterColour.image, VK_IMAGE_ASPECT_COLOR_BIT);
    copy(depthImages[parity].image, waterDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    const VkImageMemoryBarrier2 after[] = {
        barrier(sceneColour.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
        barrier(depthImages[parity].image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                depthStages, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        barrier(waterColour.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT),
        barrier(waterDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)};
    apply(cmd, after, 4);
    waterSnapshotReady = true;
}
