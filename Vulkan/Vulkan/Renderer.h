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

class Window;

class Renderer
{
public:
	Renderer();
	~Renderer();

	Window * openWindow(uint32_t size_x, uint32_t size_y, std::string name);

	bool run();

	const VkInstance getInstance() const;
	const VkPhysicalDevice getPhysicalDevice() const;
	const VkDevice getDevice() const;
	const VkQueue getQueue() const;
	const uint32_t getGraphicsFamilyIndex() const;
	const VkPhysicalDeviceProperties & getPhysicalDeviceProperties() const;

private:
	void _SetupLayersAndExtensions();

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

	Window * _window = nullptr;

	std::vector<const char *> _instance_layer_list;
	std::vector<const char *> _instance_extension_list;
	std::vector<const char *> _device_layer_list;
	std::vector<const char *> _device_extension_list;

	VkDebugReportCallbackEXT _debug_report = VK_NULL_HANDLE;
	VkDebugReportCallbackCreateInfoEXT _debug_callback_create_info {};
};

