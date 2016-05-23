#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class Renderer
{
public:
	Renderer();
	~Renderer();
private:
	void _InitInstance();
	void _DeInitInstance();

	void _InitDevice();
	void _DeInitDevice();

	void _SetupDebug();
	void _InitDebug();
	void _DeInitDebug();

	VkInstance _instance = nullptr;
	VkPhysicalDevice _gpu = nullptr;
	VkPhysicalDeviceProperties _gpu_properties = {};
	VkDevice _device = nullptr;

	uint32_t _graphics_family_index = 0;

	std::vector<const char *> _instance_layer_list;
	std::vector<const char *> _instance_extension_list;
	std::vector<const char *> _device_layer_list;
	std::vector<const char *> _device_extension_list;

	VkDebugReportCallbackEXT _debug_report = nullptr;
	VkDebugReportCallbackCreateInfoEXT _debug_callback_create_info {};
};

