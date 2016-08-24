/* Copyright (C) 2016 Daniel Grimshaw
*
* Renderer.cpp | A Vulkan based rendering object
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

#include "Renderer.h"
#include "util.h"
#include "BUILD_OPTIONS.h"
#include "Platform.h"
#include "Window.h"

#include <vulkan/vk_layer.h>

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <array>

#include <iostream>
#include <sstream>

const std::string VERT_PATH = "vert.spv";
const std::string FRAG_PATH = "frag.spv";

// Construction
Renderer::Renderer() {
	_SetupLayersAndExtensions();
	_SetupDebug();
	_InitInstance();
	_InitDebug();
	_InitDevice();
}

Renderer::~Renderer() {
	_DeInitFramebuffers();
	_DeInitGraphicsPipeline();
	_DeInitDescriptorSetLayout();
	_DeInitDescriptorPool();
	_DeInitRenderPass();
	delete _window;

	_DeInitDevice();
	_DeInitDebug();
	_DeInitInstance();
}

Window * Renderer::openWindow(uint32_t size_x, uint32_t size_y, std::string name) {
	_window = new Window(this, size_x, size_y, name);
	_InitRenderPass();
	_InitDescriptorSetLayout();
	_InitDescriptorPool();
	_InitGraphicsPipeline();
	return _window;
}

bool Renderer::run(int * xPos, int * yPos) {
	if (nullptr != _window) {
		return _window->update(xPos, yPos);
	}
	return true;
}

const VkInstance Renderer::getInstance() const {
	return _instance;
}

const VkPhysicalDevice Renderer::getPhysicalDevice() const {
	return _gpu;
}

const VkDevice Renderer::getDevice() const {
	return _device;
}

const VkQueue Renderer::getQueue() const {
	return _queue;
}

const uint32_t Renderer::getGraphicsFamilyIndex() const {
	return _graphics_family_index;
}

const VkPhysicalDeviceProperties & Renderer::getPhysicalDeviceProperties() const {
	return _gpu_properties;
}

const VkPhysicalDeviceMemoryProperties & Renderer::getPhysicalDeviceMemoryProperties() const {
	return _gpu_memory_properties;
}

const Window * Renderer::getWindow() const {
	return _window;
}

const VkRenderPass Renderer::getRenderPass() const {
	return _render_pass;
}

const std::vector<VkFramebuffer> Renderer::getSwapchainFramebuffers() const {
	return _swapchain_framebuffers;
}

const VkPipelineLayout Renderer::getPipelineLayout() const {
	return _pipeline_layout;
}

const VkPipeline Renderer::getGraphicsPipeline() const {
	return _graphics_pipeline;
}

const VkDescriptorSetLayout Renderer::getDescriptorSetLayout() const {
	return _descriptor_set_layout;
}

const VkDescriptorPool Renderer::getDescriptorPool() const {
	return _descriptor_pool;
}

void Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkBuffer & buffer, VkDeviceMemory & buffer_memory) {
	VkBufferCreateInfo buffer_create_info{};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = size;
	buffer_create_info.usage = usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	ErrorCheck(vkCreateBuffer(_device, &buffer_create_info, nullptr, &buffer));

	VkMemoryRequirements mem_requirements{};
	vkGetBufferMemoryRequirements(_device, buffer, &mem_requirements);

	uint32_t type_filter = mem_requirements.memoryTypeBits;
	uint32_t memory_type;

	for (uint32_t i = 0; i < _gpu_memory_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) && (_gpu_memory_properties.memoryTypes[i].propertyFlags & memory_properties) == memory_properties) {
			memory_type = i;
			break;
		}
	}

	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = mem_requirements.size;
	memory_allocate_info.memoryTypeIndex = memory_type;

	ErrorCheck(vkAllocateMemory(_device, &memory_allocate_info, nullptr, &buffer_memory));
	ErrorCheck(vkBindBufferMemory(_device, buffer, buffer_memory, 0));
}

void Renderer::copyBuffer(VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer command_buffer = _BeginSingleTimeCommands(commandPool);

	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = size;

	vkCmdCopyBuffer(command_buffer, srcBuffer, dstBuffer, 1, &copy_region);
	
	_EndSingleTimeCommands(commandPool, command_buffer);
}

void Renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkImage & image, VkDeviceMemory & imageMemory) {
	VkImageCreateInfo image_create_info{};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.extent.width = width;
	image_create_info.extent.height = height;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.format = format;
	image_create_info.tiling = tiling;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	image_create_info.usage = usage;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.flags = 0;

	ErrorCheck(vkCreateImage(_device, &image_create_info, nullptr, &image));

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(_device, image, &mem_reqs);

	VkMemoryAllocateInfo mem_alloc_info{};
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.allocationSize = mem_reqs.size;

	uint32_t type_filter = mem_reqs.memoryTypeBits;
	uint32_t memory_type;

	for (uint32_t i = 0; i < _gpu_memory_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) && (_gpu_memory_properties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties) {
			memory_type = i;
			break;
		}
	}
	mem_alloc_info.memoryTypeIndex = memory_type;

	ErrorCheck(vkAllocateMemory(_device, &mem_alloc_info, nullptr, &imageMemory));
	vkBindImageMemory(_device, image, imageMemory, 0);
}

void Renderer::transitionImageLayout(VkCommandPool pool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer command_buffer = _BeginSingleTimeCommands(pool);

	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else {
		throw std::invalid_argument("Unsupported layout transition.");
	}

	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	_EndSingleTimeCommands(pool, command_buffer);
}

void Renderer::copyImage(VkCommandPool pool, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) {
	VkCommandBuffer command_buffer = _BeginSingleTimeCommands(pool);

	VkImageSubresourceLayers subresource {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.baseArrayLayer = 0;
	subresource.mipLevel = 0;
	subresource.layerCount = 1;

	VkImageCopy region {};
	region.srcSubresource = subresource;
	region.dstSubresource = subresource;
	region.srcOffset = { 0, 0, 0 };
	region.dstOffset = { 0, 0, 0 };
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(
		command_buffer,
		srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region
	);

	_EndSingleTimeCommands(pool, command_buffer);
}

void Renderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView & imageView) {
	_window->createImageView(image, format, aspectFlags, imageView);
}

VkFormat Renderer::findSupportedFormat(const std::vector<VkFormat> & candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(_gpu, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("Unable to find supported format");
}

void Renderer::makeFramebuffers(VkImageView depthImageView) {
	_InitFramebuffers(depthImageView);
}
void Renderer::_SetupLayersAndExtensions() {
	//_instance_extension_list.push_back(VK_KHR_DISPLAY_EXTENSION_NAME); // Exclusive mode only
	_instance_extension_list.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	_instance_extension_list.push_back(PLATFORM_SURFACE_EXTENSION_NAME);
	
	_device_extension_list.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

// Instances
void Renderer::_InitInstance() {
	// Application info
	VkApplicationInfo application_info {};
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.apiVersion = VK_MAKE_VERSION(1, 0, 13);
	application_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	application_info.pApplicationName = "Vulkan Testing";

	// Instance create info
	VkInstanceCreateInfo instance_create_info{};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &application_info;
	instance_create_info.enabledLayerCount = (uint32_t) _instance_layer_list.size();
	instance_create_info.ppEnabledLayerNames = _instance_layer_list.data();
	instance_create_info.enabledExtensionCount = (uint32_t) _instance_extension_list.size();
	instance_create_info.ppEnabledExtensionNames = _instance_extension_list.data();
	instance_create_info.pNext = &_debug_callback_create_info;

	ErrorCheck(vkCreateInstance(&instance_create_info, nullptr, &_instance));
}

void Renderer::_DeInitInstance() {
	vkDestroyInstance(_instance, nullptr);
	_instance = nullptr;
}

// Devices
void Renderer::_InitDevice() {
	{
		uint32_t gpu_count = 0;
		
		// Read number of GPU's
		vkEnumeratePhysicalDevices(_instance, &gpu_count, nullptr);
		
		std::vector<VkPhysicalDevice> gpu_list(gpu_count);

		// Populate list
		vkEnumeratePhysicalDevices(_instance, &gpu_count, gpu_list.data());

		_gpu = gpu_list[0]; // Get the first available list
		vkGetPhysicalDeviceProperties(_gpu, &_gpu_properties);
		vkGetPhysicalDeviceMemoryProperties(_gpu, &_gpu_memory_properties);
	}
	
	{
		uint32_t family_count = 0;

		// Read number of GPU queue family properties
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, nullptr);

		std::vector<VkQueueFamilyProperties> family_property_list(family_count);

		// Populate list
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, family_property_list.data());
		
		// Find the graphics family
		bool found = false;
		for (uint32_t i = 0; i < family_count; ++i) {
			if (family_property_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				found = true;
				_graphics_family_index = i;
			}
		}

		if (!found) {
			assert(0 && "Vulkan ERROR: Queue family supporting graphics not found.");
			std::exit(-1);
		}
	}

	// Instance Layers
	{
		uint32_t layer_count = 0;

		// Read the number of layers
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> layer_property_list(layer_count);

		// Populate list
		vkEnumerateInstanceLayerProperties(&layer_count, layer_property_list.data());

#if BUILD_ENABLE_VULKAN_RUNTIME_DEBUG
		std::cout << "Instance layers: \n";
		for (auto &i : layer_property_list) {
			std::cout << " " << i.layerName << "\t\t | " << i.description << std::endl;
		}
		std::cout << std::endl;
#endif
	}

	// Instance Extensions
	{
		uint32_t extension_count = 0;

		// Read the number of extensions
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> extension_property_list(extension_count);

		// Populate list
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extension_property_list.data());

#if BUILD_ENABLE_VULKAN_RUNTIME_DEBUG
		std::cout << "Instance extensions: \n";
		for (auto &i : extension_property_list) {
			std::cout << " " << i.extensionName << "\t\t | " << i.specVersion << std::endl;
		}
		std::cout << std::endl;
#endif
	}

	// Device Layers
	{
		uint32_t layer_count = 0;

		// Read the number of layers
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, nullptr);

		std::vector<VkLayerProperties> layer_property_list(layer_count);

		// Populate list
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, layer_property_list.data());

#if BUILD_ENABLE_VULKAN_RUNTIME_DEBUG
		std::cout << "Device layers: \n";
		for (auto &i : layer_property_list) {
			std::cout << " " << i.layerName << "\t\t | " << i.description << std::endl;
		}
		std::cout << std::endl;
#endif
	}

	// Device Extensions
	{
		uint32_t extension_count = 0;

		// Read the number of extensions
		vkEnumerateDeviceExtensionProperties(_gpu, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> extension_property_list(extension_count);

		// Populate list
		vkEnumerateDeviceExtensionProperties(_gpu, nullptr, &extension_count, extension_property_list.data());

#if BUILD_ENABLE_VULKAN_RUNTIME_DEBUG
		std::cout << "Device extensions: \n";
		for (auto &i : extension_property_list) {
			std::cout << " " << i.extensionName << "\t\t | " << i.specVersion << std::endl;
		}
		std::cout << std::endl;
#endif
	}

	float queue_priorities[] {1.0f};

	VkDeviceQueueCreateInfo device_queue_create_info {};
	device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_info.queueFamilyIndex = _graphics_family_index;
	device_queue_create_info.queueCount = 1;
	device_queue_create_info.pQueuePriorities = queue_priorities;

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &device_queue_create_info;
	device_create_info.enabledLayerCount = (uint32_t) _device_layer_list.size();
	device_create_info.ppEnabledLayerNames = _device_layer_list.data();
	device_create_info.enabledExtensionCount = (uint32_t) _device_extension_list.size();
	device_create_info.ppEnabledExtensionNames = _device_extension_list.data();

	ErrorCheck(vkCreateDevice(_gpu, &device_create_info, nullptr, &_device));

	vkGetDeviceQueue(_device, _graphics_family_index, 0, &_queue);
}

void Renderer::_DeInitDevice() {
	vkDestroyDevice(_device, nullptr);
	_device = nullptr;
}

// Debug
#if BUILD_ENABLE_VULKAN_DEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugReportFlagsEXT msg_flags,
	VkDebugReportObjectTypeEXT obj_type,
	uint64_t src_obj,
	size_t location,
	int32_t msg_code,
	const char * layer_prefix,
	const char * msg,
	void * user_data
	) {
	std::ostringstream stream;

	stream << "VKDBG: ";
	if (msg_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		stream << "INFO: ";
	}
	if (msg_flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		stream << "WARNING: ";
	}
	if (msg_flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		stream << "PERFORMANCE: ";
	}
	if (msg_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		stream << "ERROR: ";
	}
	if (msg_flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		stream << "DEBUG: ";
	}

	stream << "@[" << layer_prefix << "]: ";
	stream << msg << std::endl;

	std::cout << stream.str();

#ifdef _WIN32
	if (msg_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		MessageBox(NULL, stream.str().c_str(), "Vulkan Error!", 0);
	}
#endif

	return false;
}

void Renderer::_SetupDebug() {
	_debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	_debug_callback_create_info.pfnCallback = VulkanDebugCallback;
	_debug_callback_create_info.flags =
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		0;

	_instance_layer_list.push_back("VK_LAYER_LUNARG_standard_validation");
//	_instance_layer_list.push_back("VK_LAYER_GOOGLE_threading");
//	_instance_layer_list.push_back("VK_LAYER_LUNARG_draw_state");
//	_instance_layer_list.push_back("VK_LAYER_LUNARG_image");
//	_instance_layer_list.push_back("VK_LAYER_LUNARG_mem_tracker");
//	_instance_layer_list.push_back("VK_LAYER_LUNARG_object_tracker");
//	_instance_layer_list.push_back("VK_LAYER_LUNARG_param_checker");

	_instance_extension_list.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	_device_layer_list.push_back("VK_LAYER_LUNARG_standard_validation");
//	_device_layer_list.push_back("VK_LAYER_GOOGLE_threading");
//	_device_layer_list.push_back("VK_LAYER_LUNARG_draw_state");
//	_device_layer_list.push_back("VK_LAYER_LUNARG_image");
//	_device_layer_list.push_back("VK_LAYER_LUNARG_mem_tracker");
//	_device_layer_list.push_back("VK_LAYER_LUNARG_object_tracker");
//	_device_layer_list.push_back("VK_LAYER_LUNARG_param_checker");
}

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

void Renderer::_InitDebug() {
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT");

	if (fvkCreateDebugReportCallbackEXT == nullptr || fvkDestroyDebugReportCallbackEXT == nullptr) {
		assert(0 && "Vulkan ERROR: Unable to fetch debug function pointers.");
		std::exit(-1);
	}

	fvkCreateDebugReportCallbackEXT(_instance, &_debug_callback_create_info, nullptr, &_debug_report);
}

void Renderer::_DeInitDebug() {
	fvkDestroyDebugReportCallbackEXT(_instance, _debug_report, nullptr);
	_debug_report = VK_NULL_HANDLE;
}

#else

void Renderer::_SetupDebug() {};
void Renderer::_InitDebug() {};
void Renderer::_DeInitDebug() {};
#endif // BUILD_ENABLE_VULKAN_DEBUG

void Renderer::_InitRenderPass() {
	VkAttachmentDescription color_attachment {};
	color_attachment.format = _window->getSurfaceFormat().format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment {};
	depth_attachment.format = findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_reference {};
	color_attachment_reference.attachment = 0;
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_reference {};
	depth_attachment_reference.attachment = 1;
	depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_reference;
	subpass.pDepthStencilAttachment = &depth_attachment_reference;

	VkSubpassDependency dependency {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

	VkRenderPassCreateInfo render_pass_create_info {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = (uint32_t)attachments.size();
	render_pass_create_info.pAttachments = attachments.data();
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass;
	render_pass_create_info.dependencyCount = 1;
	render_pass_create_info.pDependencies = &dependency;

	ErrorCheck(vkCreateRenderPass(_device, &render_pass_create_info, nullptr, &_render_pass));
}

void Renderer::_DeInitRenderPass() {
	vkDestroyRenderPass(_device, _render_pass, nullptr);
	_render_pass = nullptr;
}

void Renderer::_InitGraphicsPipeline() {
	{ // Create Vertex Shader Module
		std::vector<char> vert_shader_code = readFile(VERT_PATH);

		VkShaderModuleCreateInfo shader_module_create_info{};
		shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_create_info.codeSize = vert_shader_code.size();
		shader_module_create_info.pCode = (uint32_t *)vert_shader_code.data();

		ErrorCheck(vkCreateShaderModule(_device, &shader_module_create_info, nullptr, &_vert_module));
	}

	{ // Create Fragment Shader Module
		std::vector<char> frag_shader_code = readFile(FRAG_PATH);

		VkShaderModuleCreateInfo shader_module_create_info{};
		shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_create_info.codeSize = frag_shader_code.size();
		shader_module_create_info.pCode = (uint32_t *)frag_shader_code.data();

		ErrorCheck(vkCreateShaderModule(_device, &shader_module_create_info, nullptr, &_frag_module));
	}

	VkPipelineShaderStageCreateInfo vert_shader_stage_create_info{};
	vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_create_info.module = _vert_module;
	vert_shader_stage_create_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_create_info{};
	frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_create_info.module = _frag_module;
	frag_shader_stage_create_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_create_info, frag_shader_stage_create_info };

	VkVertexInputBindingDescription binding_description = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info_create_info{};
	vertex_input_info_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info_create_info.vertexBindingDescriptionCount = 1;
	vertex_input_info_create_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info_create_info.vertexAttributeDescriptionCount = (uint32_t)attribute_descriptions.size();
	vertex_input_info_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{};
	pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)(_window->getWidth());
	viewport.height = (float)(_window->getHeight());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent.width = _window->getWidth();
	scissor.extent.height = _window->getHeight();

	VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info{};
	pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipeline_viewport_state_create_info.viewportCount = 1;
	pipeline_viewport_state_create_info.pViewports = &viewport;
	pipeline_viewport_state_create_info.scissorCount = 1;
	pipeline_viewport_state_create_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.lineWidth = 1.0f;
	rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
	rasterization_state_create_info.depthBiasClamp = 0.0f;
	rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info{}; // Anti-Aliasing
	pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
	pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipeline_multisample_state_create_info.minSampleShading = 1.0f;
	pipeline_multisample_state_create_info.pSampleMask = nullptr;
	pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state{};
	pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
	pipeline_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	pipeline_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
	pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info{};
	pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
	pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
	pipeline_color_blend_state_create_info.attachmentCount = 1;
	pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;
	pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
	pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
	pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
	pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depth_stencil{};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.minDepthBounds = 0.0f;
	depth_stencil.maxDepthBounds = 1.0f;
	depth_stencil.stencilTestEnable = VK_FALSE;
	depth_stencil.front = {};
	depth_stencil.back = {};

	VkDescriptorSetLayout descriptor_set_layouts[] = { _descriptor_set_layout };
	VkPipelineLayoutCreateInfo pipeline_layout_create_info {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = 0;

	ErrorCheck(vkCreatePipelineLayout(_device, &pipeline_layout_create_info, nullptr, &_pipeline_layout));

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info {};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = 2; // Shader stages
	graphics_pipeline_create_info.pStages = shader_stages;
	graphics_pipeline_create_info.pVertexInputState = &vertex_input_info_create_info;
	graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
	graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
	graphics_pipeline_create_info.pDepthStencilState = &depth_stencil;
	graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
	graphics_pipeline_create_info.pDynamicState = nullptr;
	graphics_pipeline_create_info.layout = _pipeline_layout;
	graphics_pipeline_create_info.renderPass = _render_pass;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;

	ErrorCheck(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &_graphics_pipeline));
}

void Renderer::_DeInitGraphicsPipeline() {
	vkDestroyPipelineLayout(_device, _pipeline_layout, nullptr);
	_pipeline_layout = nullptr;
	vkDestroyShaderModule(_device, _frag_module, nullptr);
	_frag_module = nullptr;
	vkDestroyShaderModule(_device, _vert_module, nullptr);
	_vert_module = nullptr;
	vkDestroyPipeline(_device, _graphics_pipeline, nullptr);
	_graphics_pipeline = nullptr;
}

void Renderer::_InitFramebuffers(VkImageView depthImageView) {
	_swapchain_framebuffers.resize(_window->getSwapchainImageViews().size());

	for (size_t i = 0; i < _window->getSwapchainImageViews().size(); i++) {
		std::array<VkImageView, 2> attachments = {
			_window->getSwapchainImageViews()[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebuffer_create_info {};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = _render_pass;
		framebuffer_create_info.attachmentCount = (uint32_t)attachments.size();
		framebuffer_create_info.pAttachments = attachments.data();
		framebuffer_create_info.width = _window->getSurfaceCapabilities().currentExtent.width;
		framebuffer_create_info.height = _window->getSurfaceCapabilities().currentExtent.height;
		framebuffer_create_info.layers = 1;

		ErrorCheck(vkCreateFramebuffer(_device, &framebuffer_create_info, nullptr, &_swapchain_framebuffers[i]));
	}
}

void Renderer::_DeInitFramebuffers() {
	for (size_t i = 0; i < _window->getSwapchainImageViews().size(); i++) {
		vkDestroyFramebuffer(_device, _swapchain_framebuffers[i], nullptr);
		_swapchain_framebuffers[i] = nullptr;
	}
}

void Renderer::_InitDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding ubo_layout_binding {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding sampler_layout_binding {};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };
	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info {};
	descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_create_info.bindingCount = (uint32_t)bindings.size();
	descriptor_set_layout_create_info.pBindings = bindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_create_info, nullptr, &_descriptor_set_layout));
}

void Renderer::_DeInitDescriptorSetLayout() {
	vkDestroyDescriptorSetLayout(_device, _descriptor_set_layout, nullptr);
	_descriptor_set_layout = nullptr;
}

void Renderer::_InitDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> pool_sizes {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = 1;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_create_info {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.poolSizeCount = (uint32_t)pool_sizes.size();
	pool_create_info.pPoolSizes = pool_sizes.data();
	pool_create_info.maxSets = 1;

	ErrorCheck(vkCreateDescriptorPool(_device, &pool_create_info, nullptr, &_descriptor_pool));
}

void Renderer::_DeInitDescriptorPool() {
	vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
	_descriptor_pool = nullptr;
}

VkCommandBuffer Renderer::_BeginSingleTimeCommands(VkCommandPool pool) {
	VkCommandBufferAllocateInfo allocate_info {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandPool = pool;
	allocate_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	ErrorCheck(vkAllocateCommandBuffers(_device, &allocate_info, &command_buffer));

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	ErrorCheck(vkBeginCommandBuffer(command_buffer, &begin_info));

	return command_buffer;
}

void Renderer::_EndSingleTimeCommands(VkCommandPool pool, VkCommandBuffer commandBuffer) {
	ErrorCheck(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submit_info {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &commandBuffer;

	ErrorCheck(vkQueueSubmit(_queue, 1, &submit_info, VK_NULL_HANDLE));
	ErrorCheck(vkQueueWaitIdle(_queue));

	vkFreeCommandBuffers(_device, pool, 1, &commandBuffer);
}