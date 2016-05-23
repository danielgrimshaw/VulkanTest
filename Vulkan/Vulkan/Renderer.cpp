#include "Renderer.h"

#include <cstdlib>
#include <assert.h>
#include <vector>

// Construction
Renderer::Renderer() {
	_InitInstance();
	_InitDevice();
}

Renderer::~Renderer() {
	_DeInitDevice();
	_DeInitInstance();
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

	VkResult err = vkCreateInstance(&instance_create_info, nullptr, &_instance);
	if (err != VK_SUCCESS) {
		assert(0 && "Vulkan ERROR: Create instance failed.");
		std::exit(err);
	}
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
			assert(1 && "Vulkan ERROR: Queue family supporting graphics not found.");
			std::exit(-1);
		}
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

	VkResult err = vkCreateDevice(_gpu, &device_create_info, nullptr, &_device);
	if (err != VK_SUCCESS) {
		assert(1 && "Vulkan ERROR: Device creation failed.");
		std::exit(err);
	}
}

void Renderer::_DeInitDevice() {
	vkDestroyDevice(_device, nullptr);
	_device = nullptr;
}