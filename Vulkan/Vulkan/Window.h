/* Copyright (C) 2016 Daniel Grimshaw
*
* Window.h | A window for use in Vulkan
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
#include <string>
#include <cstdint>
#include <vector>

class Renderer;

class Window {
public:
	Window(Renderer * renderer, uint32_t size_x, uint32_t size_y, std::string name);
	~Window();

	void close();
	bool update();

	const uint32_t getWidth() const;
	const uint32_t getHeight() const;
	const VkSurfaceCapabilitiesKHR getSurfaceCapabilities() const;
	const VkSurfaceFormatKHR getSurfaceFormat() const;
	const VkSwapchainKHR getSwapchain() const;
	const std::vector<VkImage> & getSwapchainImages() const;
	const std::vector<VkImageView> & getSwapchainImageViews() const;

private:
	void _InitOSWindow();
	void _DeInitOSWindow();
	void _UpdateOSWindow();
	void _InitOSSurface();

	void _InitSurface();
	void _DeInitSurface();

	void _InitSwapchain();
	void _DeInitSwapchain();

	void _InitSwapchainImages();
	void _DeInitSwapchainImages();

	Renderer * _renderer = nullptr;

	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

	uint32_t _surface_size_x = 512;
	uint32_t _surface_size_y = 512;
	std::string _window_name;
	uint32_t _swapchain_image_count = 2;

	VkSurfaceFormatKHR _surface_format = {};
	VkSurfaceCapabilitiesKHR _surface_capabilities = {};

	std::vector<VkImage> _swapchain_images;
	std::vector<VkImageView> _swapchain_image_views;

	bool _window_should_run = true;

#if VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE _win32_instance = NULL;
	HWND _win32_window = NULL;
	std::string _win32_class_name;
	static uint64_t _win32_class_id_counter;
#endif
};

