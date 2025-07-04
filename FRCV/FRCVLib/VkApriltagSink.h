#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>
#include <opencv2/opencv.hpp>

class VkApriltagSink {
public:
    VkApriltagSink(uint32_t width,
        uint32_t height,
        uint32_t decimationFactor);
    ~VkApriltagSink();

    // Initializes Vulkan, pipelines, and resources
    void initialize();

    // Uploads a frame (RGBA Mat), runs detection, and returns poses
    std::vector<cv::Vec3d> processFrame(const cv::Mat& rgbaFrame);

private:
    // pipeline steps
    void initVulkan();
    void buildResources();
    void buildPipelines();
    void recordCommandBuffer();
    void cleanup();

    // configuration
    uint32_t WIDTH;
    uint32_t HEIGHT;
    uint32_t DEC_FAC;
    static constexpr uint32_t MAX_COMPS = 1024;

    // Vulkan objects
    VkInstance              instance{};
    VkPhysicalDevice        physicalDevice{};
    VkDevice                device{};
    VkQueue                 queue{};
    uint32_t                queueFamilyIndex{};
    VkCommandPool           cmdPool{};
    VkCommandBuffer         cmdBuf{};
    VkFence                 fence{};

    VkBuffer                bufStaging{};
    VkDeviceMemory          memStaging{};
    VkImage                 imgInput{};
    VkImage                 imgGray{};
    VkImage                 imgBinary{};
    VkImage                 imgLabel{};
    VkDeviceMemory          memInput{};
    VkDeviceMemory          memGray{};
    VkDeviceMemory          memBinary{};
    VkDeviceMemory          memLabel{};
    VkBuffer                bufQuads{};
    VkDeviceMemory          memQuads{};

    VkDescriptorSetLayout   dsLayout{};
    VkPipelineLayout        pipelineLayout{};
    VkPipeline              pipeGray{};
    VkPipeline              pipeThresh{};
    VkPipeline              pipeCCL{};
    VkPipeline              pipeQuads{};
    VkDescriptorPool        dsPool{};
    VkDescriptorSet         dsGray{};
    VkDescriptorSet         dsThresh{};
    VkDescriptorSet         dsCCL{};
    VkDescriptorSet         dsQuads{};
};