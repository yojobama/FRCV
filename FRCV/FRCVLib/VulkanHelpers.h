#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

void chackVk(VkResult result, const char* msg);

VkShaderModule locdShaderModule(
	VkDevice device,
	const std::string& filePath
);

uint32_t findMemoryType(
	VkPhysicalDevice physicalDevice,
	uint32_t typeFilter,
	VkMemoryPropertyFlags properties
);