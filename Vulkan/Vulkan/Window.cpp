#include "Window.h"
#include "Renderer.h"
#include <cstdint>
#include <assert.h>


Window::Window(Renderer * renderer, uint32_t size_x, uint32_t size_y, std::string name) {
	_renderer = renderer;
	_surface_size_x = size_x;
	_surface_size_y = size_y;
	_window_name = name;

	_InitOSWindow();
	_InitSurface();
	_InitSwapchain();
}


Window::~Window() {
	_DeInitSwapchain();
	_DeInitOSWindow();
	_DeInitSurface();
}

void Window::close() {
	_window_should_run = false;
}

bool Window::update() {
	_UpdateOSWindow();
	return _window_should_run;
}

void Window::_InitSurface() {
	_InitOSSurface();

	VkBool32 WSI_support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(_renderer->getPhysicalDevice(), _renderer->getGraphicsFamilyIndex(), _surface, &WSI_support);
	if (!WSI_support) {
		assert(0 && "WSI not supported");
		std::exit(-1);
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_renderer->getPhysicalDevice(), _surface, &_surface_capabilities);

	if (_surface_capabilities.currentExtent.width < UINT32_MAX) {
		_surface_size_x = _surface_capabilities.currentExtent.width;
		_surface_size_y = _surface_capabilities.currentExtent.height;
	}

	{
		uint32_t format_count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(_renderer->getPhysicalDevice(), _surface, &format_count, nullptr);

		if (format_count == 0) {
			assert(0 && "No possible surface formats");
			std::exit(-1);
		}

		std::vector<VkSurfaceFormatKHR> formats(format_count);

		vkGetPhysicalDeviceSurfaceFormatsKHR(_renderer->getPhysicalDevice(), _surface, &format_count, formats.data());
		if (formats[0].format == VK_FORMAT_UNDEFINED) {
			_surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
			_surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		}
		else {
			_surface_format = formats[0];
		}
	}
}

void Window::_DeInitSurface() {
	vkDestroySurfaceKHR(_renderer->getInstance(), _surface, nullptr);
}
