// VulkanTesting.cpp : Defines the entry point for the console application.
//
#define VK_USE_PLATFORM_WIN32_KHR

#include "stdafx.h"

void vulkanRender();

int WinMain(_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow) {
	HWND hwnd;

	hwnd = CreateWindowEx(NULL,
		"Main",
		"Vulkan Practice",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0,
		800, 600,
		NULL,
		NULL,
		hInstance,
		NULL);
	if (!hwnd) {
		fprintf(stderr, "Vulkan Practice: Error: Could not create window instance! Abort.");
		exit(1);
	}
	vulkanRender(hInstance, hwnd);
	return 0;
}

void vulkanRender(HINSTANCE hInst, HWND hwnd) {
	const char * extensionNames[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };

	VkInstanceCreateInfo instanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,	// VkStructureType sType;
		.pNext = NULL,										// const void * pNext
		.flags = 0,											// VkInstanceCreateFlags flags;
		.pApplicationInfo = NULL,							// const VkApplicationInfo * pApplicationInfo
		.enabledLayerCount = 0,								// uint32_t enabledLayerCount;
		.ppEnabledLayerNames = NULL,						// const char * const * ppEnabledLayerNames;
		.enabledExtensionCount = 2,							// uint32_t enabledExtensionCount;
		.ppEnabledExtensionNames = extensionNames,			// const char * const * ppEnabledExtensionNames;
	};

	VkInstance inst;
	vkCreateInstance(&instanceCreateInfo, NULL, &inst);

	VkPhysicalDevice * phys;
	uint32_t physCount;
	vkEnumeratePhysicalDevices(inst, &physCount, NULL);
	phys = malloc(sizeof(VkPhysicalDevice) * physCount);
	vkEnumeratePhysicalDevices(inst, &physCount, phys);

	VkDeviceCreateInfo deviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = NULL,
		.pQueueCreateInfos = NULL,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 2,
		.ppEnabledExtensionNames = extensionNames,
		.pEnabledFeatures = NULL,
	};

	VkDevice dev;
	vkCreateDevice(phys[0], &deviceCreateInfo, NULL, &dev);

	// fetch vkCreateWin32SurfaceKHR extension function pointer via vkGetInstanceProcAddr
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.hinstance = hInst,
		.hwnd = hwnd,
	};

	VkSurfaceKHR surf;
	vkCreateWin32SurfaceKHR(inst, &surfaceCreateInfo, NULL, &surf);

	VkSwapchainCreateInfoKHR swapCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = surf,
		.minImageCount = 0,
		//TODO
	};
}

