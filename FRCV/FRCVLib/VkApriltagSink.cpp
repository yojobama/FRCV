#include "VkApriltagSink.h"
#include "VulkanHelpers.h"
#include <vector>
#include <array>
#include <cassert>

VkApriltagSink::VkApriltagSink(uint32_t width,
    uint32_t height,
    uint32_t decimationFactor)
    : WIDTH(width), HEIGHT(height), DEC_FAC(decimationFactor) {
}

VkApriltagSink::~VkApriltagSink() {
    cleanup();
}

void VkApriltagSink::initialize() {
    initVulkan();
    buildResources();
    buildPipelines();
    // record once—reused each frame
    recordCommandBuffer();
}

std::vector<cv::Vec3d> VkApriltagSink::processFrame(const cv::Mat& rgbaFrame) {
    // 1) Upload
    void* ptr = nullptr;
    vkMapMemory(device, memStaging, 0, VK_WHOLE_SIZE, 0, &ptr);
    memcpy(ptr, rgbaFrame.data, rgbaFrame.total() * 4);
    vkUnmapMemory(device, memStaging);

    // 2) Submit
    vkResetFences(device, 1, &fence);
    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmdBuf;
    vkQueueSubmit(queue, 1, &si, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    // 3) Read back quads
    vkMapMemory(device, memQuads, 0, VK_WHOLE_SIZE, 0, &ptr);
    struct Quad { float corners[8]; uint32_t count; };
    auto quads = reinterpret_cast<Quad*>(ptr);

    std::vector<cv::Vec3d> poses;
    for (uint32_t i = 0; i < quads->count && i < MAX_COMPS; ++i) {
        // sample corners, decode, solvePnP...
        // placeholder: return zero pose
        poses.emplace_back(0, 0, 0);
    }
    vkUnmapMemory(device, memQuads);
    return poses;
}

// — Implementation of initVulkan, buildResources, buildPipelines,
//   recordCommandBuffer, cleanup follows exactly the code patterns
//   from the previous main.cpp, only using member variables instead
//   of globals. For brevity, refer to the earlier skeleton, wrapping
//   each function body into the class namespace.

void VkApriltagSink::initVulkan() {
    // create instance, pick device, queue, create cmdPool, cmdBuf, fence...
}

void VkApriltagSink::buildResources() {
    // create staging buffer, images, SSBOs with VulkanHelpers functions...
}

void VkApriltagSink::buildPipelines() {
    // load shaders, create descriptor layouts/pools, pipelines, descriptor sets...
}

void VkApriltagSink::recordCommandBuffer() {
    // record dispatches gray → thresh → ccl → extract_quads with barriers...
}

void VkApriltagSink::cleanup() {
    // destroy pipelines, layouts, buffers, images, Vulkan objects...
}