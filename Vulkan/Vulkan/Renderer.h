/* Copyright (C) 2016 Daniel Grimshaw
*
* Renderer.h | A Vulkan based rendering object
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Platform.h"

#include <vector>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription binding_description {};
		binding_description.binding = 0;
		binding_description.stride = sizeof(Vertex);
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return binding_description;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions;
		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[0].offset = offsetof(Vertex, pos);

		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[1].offset = offsetof(Vertex, color);

		attribute_descriptions[2].binding = 0;
		attribute_descriptions[2].location = 2;
		attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[2].offset = offsetof(Vertex, texCoord);

		return attribute_descriptions;
	}

	bool operator==(const Vertex & other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const & vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct UniformBufferObject { // UBO
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

class Window;

class Renderer
{
public:
	Renderer();
	~Renderer();

	Window * openWindow(uint32_t size_x, uint32_t size_y, std::string name);

	bool run(int * xPos, int * yPos);

	const VkInstance getInstance() const;
	const VkPhysicalDevice getPhysicalDevice() const;
	const VkDevice getDevice() const;
	const VkQueue getQueue() const;
	const uint32_t getGraphicsFamilyIndex() const;
	const VkPhysicalDeviceProperties & getPhysicalDeviceProperties() const;
	const VkPhysicalDeviceMemoryProperties & getPhysicalDeviceMemoryProperties() const;
	const Window * getWindow() const;
	const VkRenderPass getRenderPass() const;
	const std::vector<VkFramebuffer> getSwapchainFramebuffers() const;
	const VkPipelineLayout getPipelineLayout() const;
	const VkPipeline getGraphicsPipeline() const;
	const VkDescriptorSetLayout getDescriptorSetLayout() const;
	const VkDescriptorPool getDescriptorPool() const;

	void makeFramebuffers(VkImageView depthImageView);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkBuffer & buffer, VkDeviceMemory & buffer_memory);
	void copyBuffer(VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory);
	void transitionImageLayout(VkCommandPool pool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyImage(VkCommandPool pool, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height);
	void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView & imageView);

	VkFormat findSupportedFormat(const std::vector<VkFormat> & candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

private:
	void _SetupLayersAndExtensions();

	void _InitInstance();
	void _DeInitInstance();

	void _InitDevice();
	void _DeInitDevice();

	void _SetupDebug();
	void _InitDebug();
	void _DeInitDebug();

	void _InitGraphicsPipeline();
	void _DeInitGraphicsPipeline();

	void _InitRenderPass();
	void _DeInitRenderPass();

	void _InitFramebuffers(VkImageView depthImageView);
	void _DeInitFramebuffers();

	void _InitDescriptorSetLayout();
	void _DeInitDescriptorSetLayout();

	void _InitDescriptorPool();
	void _DeInitDescriptorPool();

	VkCommandBuffer _BeginSingleTimeCommands(VkCommandPool pool);
	void _EndSingleTimeCommands(VkCommandPool pool, VkCommandBuffer commandBuffer);

	VkInstance _instance = VK_NULL_HANDLE;
	VkPhysicalDevice _gpu = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties _gpu_properties = {};
	VkPhysicalDeviceMemoryProperties _gpu_memory_properties = {};
	VkDevice _device = VK_NULL_HANDLE;
	VkQueue _queue = VK_NULL_HANDLE;
	VkShaderModule _vert_module;
	VkShaderModule _frag_module;
	VkDescriptorSetLayout _descriptor_set_layout;
	VkDescriptorPool _descriptor_pool;
	VkPipelineLayout _pipeline_layout;
	VkRenderPass _render_pass;
	VkPipeline _graphics_pipeline;
	std::vector<VkFramebuffer> _swapchain_framebuffers;

	uint32_t _graphics_family_index = 0;

	Window * _window = nullptr;

	std::vector<const char *> _instance_layer_list;
	std::vector<const char *> _instance_extension_list;
	std::vector<const char *> _device_layer_list;
	std::vector<const char *> _device_extension_list;

	VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
	VkDebugReportCallbackCreateInfoEXT _debug_callback_create_info {};
};

