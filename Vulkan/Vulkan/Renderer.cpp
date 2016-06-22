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
	delete _window;

	_DeInitDevice();
	_DeInitDebug();
	_DeInitInstance();
}

Window * Renderer::openWindow(uint32_t size_x, uint32_t size_y, std::string name) {
	_window = new Window(this, size_x, size_y, name);
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

void Renderer::_SetupLayersAndExtensions() {
	//_instance_extension_list.push_back(VK_KHR_DISPLAY_EXTENSION_NAME); // Exclusive mode only
	_instance_extension_list.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	_instance_extension_list.push_back(PLATFORM_SURFACE_EXTENSION_NAME);
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
	instance_create_info.enabledLayerCount = _instance_layer_list.size();
	instance_create_info.ppEnabledLayerNames = _instance_layer_list.data();
	instance_create_info.enabledExtensionCount = _instance_extension_list.size();
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

		std::cout << "Instance layers: \n";
		for (auto &i : layer_property_list) {
			std::cout << " " << i.layerName << "\t\t\ | " << i.description << std::endl;
		}
		std::cout << std::endl;
	}

	// Instance Extensions
	{
		uint32_t extension_count = 0;

		// Read the number of extensions
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> extension_property_list(extension_count);

		// Populate list
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extension_property_list.data());

		std::cout << "Instance extensions: \n";
		for (auto &i : extension_property_list) {
			std::cout << " " << i.extensionName << "\t\t\ | " << i.specVersion << std::endl;
		}
		std::cout << std::endl;
	}

	// Device Layers
	{
		uint32_t layer_count = 0;

		// Read the number of layers
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, nullptr);

		std::vector<VkLayerProperties> layer_property_list(layer_count);

		// Populate list
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, layer_property_list.data());

		std::cout << "Device layers: \n";
		for (auto &i : layer_property_list) {
			std::cout << " " << i.layerName << "\t\t\ | " << i.description << std::endl;
		}
		std::cout << std::endl;
	}

	// Device Extensions
	{
		uint32_t extension_count = 0;

		// Read the number of extensions
		vkEnumerateDeviceExtensionProperties(_gpu, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> extension_property_list(extension_count);

		// Populate list
		vkEnumerateDeviceExtensionProperties(_gpu, nullptr, &extension_count, extension_property_list.data());

		std::cout << "Device extensions: \n";
		for (auto &i : extension_property_list) {
			std::cout << " " << i.extensionName << "\t\t\ | " << i.specVersion << std::endl;
		}
		std::cout << std::endl;
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
	device_create_info.enabledLayerCount = _device_layer_list.size();
	device_create_info.ppEnabledLayerNames = _device_layer_list.data();
	device_create_info.enabledExtensionCount = _device_extension_list.size();
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