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

#include <iostream>
#include <sstream>

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
	_DeInitRenderPass();
	delete _window;

	_DeInitDevice();
	_DeInitDebug();
	_DeInitInstance();
}

Window * Renderer::openWindow(uint32_t size_x, uint32_t size_y, std::string name) {
	_window = new Window(this, size_x, size_y, name);
	_InitRenderPass();
	_InitGraphicsPipeline();
	_InitFramebuffers();
	return _window;
}

bool Renderer::run() {
	if (nullptr != _window) {
		return _window->update();
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

const VkPipeline Renderer::getGraphicsPipeline() const {
	return _graphics_pipeline;
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
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = _window->getSurfaceFormat().format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_reference {};
	color_attachment_reference.attachment = 0;
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_reference;

	VkRenderPassCreateInfo render_pass_create_info {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = 1;
	render_pass_create_info.pAttachments = &color_attachment;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass;

	ErrorCheck(vkCreateRenderPass(_device, &render_pass_create_info, nullptr, &_render_pass));
}

void Renderer::_DeInitRenderPass() {
	vkDestroyRenderPass(_device, _render_pass, nullptr);
	_render_pass = nullptr;
}

void Renderer::_InitGraphicsPipeline() {
	VkShaderModule vert_module;
	VkShaderModule frag_module;

	{ // Create Vertex Shader Module
		std::vector<char> vert_shader_code = readFile("vert.spv");

		VkShaderModuleCreateInfo shader_module_create_info {};
		shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_create_info.codeSize = vert_shader_code.size();
		shader_module_create_info.pCode = (uint32_t *)vert_shader_code.data();

		ErrorCheck(vkCreateShaderModule(_device, &shader_module_create_info, nullptr, &vert_module));
	}

	{ // Create Fragment Shader Module
		std::vector<char> frag_shader_code = readFile("frag.spv");

		VkShaderModuleCreateInfo shader_module_create_info{};
		shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_create_info.codeSize = frag_shader_code.size();
		shader_module_create_info.pCode = (uint32_t *)frag_shader_code.data();

		ErrorCheck(vkCreateShaderModule(_device, &shader_module_create_info, nullptr, &frag_module));
	}

	VkPipelineShaderStageCreateInfo vert_shader_stage_create_info {};
	vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_create_info.module = vert_module;
	vert_shader_stage_create_info.pName = "main";
	
	VkPipelineShaderStageCreateInfo frag_shader_stage_create_info {};
	frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_create_info.module = frag_module;
	frag_shader_stage_create_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_create_info, frag_shader_stage_create_info };

	VkVertexInputBindingDescription binding_description = Vertex::getBindingDescription();
	std::vector<VkVertexInputAttributeDescription> attribute_description = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info_create_info {};
	vertex_input_info_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info_create_info.vertexBindingDescriptionCount = 1;
	vertex_input_info_create_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info_create_info.vertexAttributeDescriptionCount = 1;
	vertex_input_info_create_info.pVertexAttributeDescriptions = attribute_description.data();

	VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info {};
	pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)(_window->getWidth());
	viewport.height = (float)(_window->getHeight());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor {};
	scissor.offset = { 0, 0 };
	scissor.extent.width = _window->getWidth();
	scissor.extent.height = _window->getHeight();

	VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info {};
	pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipeline_viewport_state_create_info.viewportCount = 1;
	pipeline_viewport_state_create_info.pViewports = &viewport;
	pipeline_viewport_state_create_info.scissorCount = 1;
	pipeline_viewport_state_create_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.lineWidth = 1.0f;
	rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
	rasterization_state_create_info.depthBiasClamp = 0.0f;
	rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info {}; // Anti-Aliasing
	pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
	pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipeline_multisample_state_create_info.minSampleShading = 1.0f;
	pipeline_multisample_state_create_info.pSampleMask = nullptr;
	pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state {};
	pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
	pipeline_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	pipeline_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
	pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info {};
	pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
	pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
	pipeline_color_blend_state_create_info.attachmentCount = 1;
	pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;
	pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
	pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
	pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
	pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;

	VkPipelineLayout pipeline_layout;

	VkPipelineLayoutCreateInfo pipeline_layout_create_info {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 0;
	pipeline_layout_create_info.pSetLayouts = nullptr;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = 0;

	ErrorCheck(vkCreatePipelineLayout(_device, &pipeline_layout_create_info, nullptr, &pipeline_layout));

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info {};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = 2; // Shader stages
	graphics_pipeline_create_info.pStages = shader_stages;
	graphics_pipeline_create_info.pVertexInputState = &vertex_input_info_create_info;
	graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
	graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
	graphics_pipeline_create_info.pDepthStencilState = nullptr;
	graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
	graphics_pipeline_create_info.pDynamicState = nullptr;
	graphics_pipeline_create_info.layout = pipeline_layout;
	graphics_pipeline_create_info.renderPass = _render_pass;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;

	ErrorCheck(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &_graphics_pipeline));

	// Might need to be moved to the destroy function
	vkDestroyPipelineLayout(_device, pipeline_layout, nullptr);
	pipeline_layout = nullptr;
	vkDestroyShaderModule(_device, frag_module, nullptr);
	frag_module = nullptr;
	vkDestroyShaderModule(_device, vert_module, nullptr);
	vert_module = nullptr;
}

void Renderer::_DeInitGraphicsPipeline() {
	vkDestroyPipeline(_device, _graphics_pipeline, nullptr);
	_graphics_pipeline = nullptr;
}

void Renderer::_InitFramebuffers() {
	_swapchain_framebuffers.resize(_window->getSwapchainImageViews().size());

	for (size_t i = 0; i < _window->getSwapchainImageViews().size(); i++) {
		VkImageView attachments[] = {
			_window->getSwapchainImageViews()[i]
		};

		VkFramebufferCreateInfo framebuffer_create_info {};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = _render_pass;
		framebuffer_create_info.attachmentCount = 1;
		framebuffer_create_info.pAttachments = attachments;
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