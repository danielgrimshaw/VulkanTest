#include "Window.h"
#include "Renderer.h"
#include "util.h"
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

void Window::_InitSwapchain() {
	if (_swapchain_image_count < _surface_capabilities.minImageCount)
		_swapchain_image_count = _surface_capabilities.minImageCount + 1;
	if (_swapchain_image_count > _surface_capabilities.maxImageCount)
		_swapchain_image_count = _surface_capabilities.maxImageCount;
	
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	{
		uint32_t present_mode_count = 0;
		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->getPhysicalDevice(), _surface, &present_mode_count, nullptr));

		std::vector<VkPresentModeKHR> present_mode_list(present_mode_count);

		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->getPhysicalDevice(), _surface, &present_mode_count, present_mode_list.data()));

		for (auto m : present_mode_list) {
			if (m == VK_PRESENT_MODE_MAILBOX_KHR)
				present_mode = m;
		}
	}

	VkSwapchainCreateInfoKHR swapchain_create_info {};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = _surface;
	swapchain_create_info.minImageCount = _swapchain_image_count; // Buffers
	swapchain_create_info.imageFormat = _surface_format.format;
	swapchain_create_info.imageColorSpace = _surface_format.colorSpace;
	swapchain_create_info.imageExtent.width = _surface_size_x;
	swapchain_create_info.imageExtent.height = _surface_size_y;
	swapchain_create_info.imageArrayLayers = 1; // Number of images (mono, stereo)
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 0;
	swapchain_create_info.pQueueFamilyIndices = nullptr;
	swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode = present_mode;
	swapchain_create_info.clipped = VK_TRUE;
	swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

	ErrorCheck(vkCreateSwapchainKHR(_renderer->getDevice(), &swapchain_create_info, nullptr, &_swapchain));

	ErrorCheck(vkGetSwapchainImagesKHR(_renderer->getDevice(), _swapchain, &_swapchain_image_count, nullptr));
}

void Window::_DeInitSwapchain() {
	vkDestroySwapchainKHR(_renderer->getDevice(), _swapchain, nullptr);
}

void Window::_DeInitSurface() {
	vkDestroySurfaceKHR(_renderer->getInstance(), _surface, nullptr);
}
