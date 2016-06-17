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

#include <vulkan/vulkan.h>
#include <vector>

class Renderer
{
public:
	Renderer();
	~Renderer();

	VkInstance getInstance();
	VkPhysicalDevice getPhysicalDevice();
	VkPhysicalDeviceProperties getPhysicalDeviceProperties();
	VkDevice getDevice();
	VkQueue getQueue();

	uint32_t getGraphicsFamilyIndex();

private:
	void _InitInstance();
	void _DeInitInstance();

	void _InitDevice();
	void _DeInitDevice();

	void _SetupDebug();
	void _InitDebug();
	void _DeInitDebug();

	VkInstance _instance = VK_NULL_HANDLE;
	VkPhysicalDevice _gpu = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties _gpu_properties = {};
	VkDevice _device = VK_NULL_HANDLE;
	VkQueue _queue = VK_NULL_HANDLE;

	uint32_t _graphics_family_index = 0;

	std::vector<const char *> _instance_layer_list;
	std::vector<const char *> _instance_extension_list;
	std::vector<const char *> _device_layer_list;
	std::vector<const char *> _device_extension_list;

	VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
	VkDebugReportCallbackCreateInfoEXT _debug_callback_create_info {};
};

