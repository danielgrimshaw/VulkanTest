// VulkanTesting.cpp : Defines the entry point for the console application.
//
#define VK_USE_PLATFORM_WIN32_KHR

#include "VulkanTesting.h"
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

#ifdef _WIN32
static void init(DG_Window *window, HINSTANCE hInstance, LPSTR pCmdLine)
#else  // _WIN32
static void init(DG_Window *window, const int argc, const char *argv[])
#endif // _WIN32
{
	bool argv_error = false;

	memset(window, 0, sizeof(*window));

#ifdef _WIN32
	window->connection = hInstance;
	strncpy(window->name, "Vulkan Testing", APP_NAME_STR_LEN);

	if (strncmp(pCmdLine, "--use_staging", strlen("--use_staging")) == 0)
		window->use_staging_buffer = true;
	else if (strlen(pCmdLine) != 0) {
		fprintf(stderr, "Do not recognize argument \"%s\".\n", pCmdLine);
		argv_error = true;
	}
#else  // _WIN32
	for (int i = 0; i < argc; i++) {
		if (strncmp(argv[i], "--use_staging", strlen("--use_staging")) == 0)
			window->use_staging_buffer = true;
	}
#endif // _WIN32
	if (argv_error) {
		fprintf(stderr, "Usage:\n  %s [--use_staging]\n", "Vulkan Testing");
		fflush(stderr);
		exit(1);
	}

	init_connection(window);
	init_vk(window);

	window->width = 800;
	window->height = 600;
	window->depthStencil = 1.0;
	window->depthIncrement = -0.01f;
}

static void init_connection(DG_Window *window) {
#ifndef _WIN32
	const xcb_setup_t *setup;
	xcb_screen_iterator_t iter;
	int scr;

	window->connection = xcb_connect(NULL, &scr);
	if (window->connection == NULL) {
		printf("Cannot find a compatible Vulkan installable client driver "
			"(ICD).\nExiting ...\n");
		fflush(stdout);
		exit(1);
	}

	setup = xcb_get_setup(window->connection);
	iter = xcb_setup_roots_iterator(setup);
	while (scr-- > 0)
		xcb_screen_next(&iter);

	window->screen = iter.data;
#endif // _WIN32
}

static void init_vk(DG_Window *window) {
	VkResult err;
	uint32_t instance_extension_count = 0;
	uint32_t instance_layer_count = 0;
	uint32_t device_validation_layer_count = 0;
	window->enabled_extension_count = 0;
	window->enabled_layer_count = 0;

	char *instance_validation_layers[] = {
		"VK_LAYER_LUNARG_mem_tracker",
		"VK_LAYER_GOOGLE_unique_objects",
	};

	window->device_validation_layers[0] = "VK_LAYER_LUNARG_mem_tracker";
	window->device_validation_layers[1] = "VK_LAYER_GOOGLE_unique_objects";
	device_validation_layer_count = 2;

	/* Look for validation layers */
	VkBool32 validation_found = 0;
	err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
	assert(!err);

	if (instance_layer_count > 0) {
		VkLayerProperties *instance_layers =
			malloc(sizeof(VkLayerProperties) * instance_layer_count);
		err = vkEnumerateInstanceLayerProperties(&instance_layer_count,
			instance_layers);
		assert(!err);

		if (window->validate) {
			validation_found = check_layers(
				ARRAY_SIZE(instance_validation_layers),
				instance_validation_layers, instance_layer_count,
				instance_layers);
			window->enabled_layer_count = ARRAY_SIZE(instance_validation_layers);
		}

		free(instance_layers);
	}

	if (window->validate && !validation_found) {
		ERR_EXIT("vkEnumerateInstanceLayerProperties failed to find"
			"required validation layer.\n\n",
			"vkCreateInstance Failure");
	}

	/* Look for instance extensions */
	VkBool32 surfaceExtFound = 0;
	VkBool32 platformSurfaceExtFound = 0;
	memset(window->extension_names, 0, sizeof(window->extension_names));

	err = vkEnumerateInstanceExtensionProperties(
		NULL, &instance_extension_count, NULL);
	assert(!err);

	if (instance_extension_count > 0) {
		VkExtensionProperties *instance_extensions =
			malloc(sizeof(VkExtensionProperties) * instance_extension_count);
		err = vkEnumerateInstanceExtensionProperties(
			NULL, &instance_extension_count, instance_extensions);
		assert(!err);
		for (uint32_t i = 0; i < instance_extension_count; i++) {
			if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME,
				instance_extensions[i].extensionName)) {
				surfaceExtFound = 1;
				window->extension_names[window->enabled_extension_count++] =
					VK_KHR_SURFACE_EXTENSION_NAME;
			}
#ifdef _WIN32
			if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
				instance_extensions[i].extensionName)) {
				platformSurfaceExtFound = 1;
				window->extension_names[window->enabled_extension_count++] =
					VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
			}
#else  // _WIN32
			if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME,
				instance_extensions[i].extensionName)) {
				platformSurfaceExtFound = 1;
				window->extension_names[window->enabled_extension_count++] =
					VK_KHR_XCB_SURFACE_EXTENSION_NAME;
			}
#endif // _WIN32
			if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
				instance_extensions[i].extensionName)) {
				if (window->validate) {
					window->extension_names[window->enabled_extension_count++] =
						VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
				}
			}
			assert(window->enabled_extension_count < 64);
		}

		free(instance_extensions);
	}

	if (!surfaceExtFound) {
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
			"the " VK_KHR_SURFACE_EXTENSION_NAME
			" extension.\n\nDo you have a compatible "
			"Vulkan installable client driver (ICD) installed?\n",
			"vkCreateInstance Failure");
	}
	if (!platformSurfaceExtFound) {
#ifdef _WIN32
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
			"the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
			" extension.\n\nDo you have a compatible "
			"Vulkan installable client driver (ICD) installed?\n",
			"vkCreateInstance Failure");
#else  // _WIN32
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
			"the " VK_KHR_XCB_SURFACE_EXTENSION_NAME
			" extension.\n\nDo you have a compatible "
			"Vulkan installable client driver (ICD) installed?\n",
			"vkCreateInstance Failure");
#endif // _WIN32
	}
	const VkApplicationInfo app = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "Vulkan Testing",
		.applicationVersion = 0,
		.pEngineName = "Vulkan Testing",
		.engineVersion = 0,
		.apiVersion = VK_API_VERSION,
	};
	VkInstanceCreateInfo inst_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.pApplicationInfo = &app,
		.enabledLayerCount = window->enabled_layer_count,
		.ppEnabledLayerNames = (const char *const *)instance_validation_layers,
		.enabledExtensionCount = window->enabled_extension_count,
		.ppEnabledExtensionNames = (const char *const *)window->extension_names,
	};

	uint32_t gpu_count;

	window->allocator.pfnAllocation = myalloc;
	window->allocator.pfnFree = myfree;
	window->allocator.pfnReallocation = myrealloc;

	err = vkCreateInstance(&inst_info, &window->allocator, &window->inst);
	if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
		ERR_EXIT("Cannot find a compatible Vulkan installable client driver "
			"(ICD).\n\n",
			"vkCreateInstance Failure");
	}
	else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
		ERR_EXIT("Cannot find a specified extension library"
			".\nMake sure your layers path is set appropriately\n",
			"vkCreateInstance Failure");
	}
	else if (err) {
		ERR_EXIT("vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
			"installable client driver (ICD) installed?\n",
			"vkCreateInstance Failure");
	}

	/* Make initial call to query gpu_count, then second call for gpu info*/
	err = vkEnumeratePhysicalDevices(window->inst, &gpu_count, NULL);
	assert(!err && gpu_count > 0);

	if (gpu_count > 0) {
		VkPhysicalDevice *physical_devices =
			malloc(sizeof(VkPhysicalDevice) * gpu_count);
		err = vkEnumeratePhysicalDevices(window->inst, &gpu_count,
			physical_devices);
		assert(!err);
		/* Just grab the first physical device */
		window->gpu = physical_devices[0];
		free(physical_devices);
	}
	else {
		ERR_EXIT("vkEnumeratePhysicalDevices reported zero accessible devices."
			"\n\nDo you have a compatible Vulkan installable client"
			" driver (ICD) installed?\n",
			"vkEnumeratePhysicalDevices Failure");
	}

	/* Look for validation layers */
	validation_found = 0;
	window->enabled_layer_count = 0;
	uint32_t device_layer_count = 0;
	err =
		vkEnumerateDeviceLayerProperties(window->gpu, &device_layer_count, NULL);
	assert(!err);

	if (device_layer_count > 0) {
		VkLayerProperties *device_layers =
			malloc(sizeof(VkLayerProperties) * device_layer_count);
		err = vkEnumerateDeviceLayerProperties(window->gpu, &device_layer_count,
			device_layers);
		assert(!err);

		if (window->validate) {
			validation_found = check_layers(device_validation_layer_count,
				window->device_validation_layers,
				device_layer_count,
				device_layers);
			window->enabled_layer_count = device_validation_layer_count;
		}

		free(device_layers);
	}

	if (window->validate && !validation_found) {
		ERR_EXIT("vkEnumerateDeviceLayerProperties failed to find "
			"a required validation layer.\n\n",
			"vkCreateDevice Failure");
	}

	/* Loog for device extensions */
	uint32_t device_extension_count = 0;
	VkBool32 swapchainExtFound = 0;
	window->enabled_extension_count = 0;
	memset(window->extension_names, 0, sizeof(window->extension_names));

	err = vkEnumerateDeviceExtensionProperties(window->gpu, NULL,
		&device_extension_count, NULL);
	assert(!err);

	if (device_extension_count > 0) {
		VkExtensionProperties *device_extensions =
			malloc(sizeof(VkExtensionProperties) * device_extension_count);
		err = vkEnumerateDeviceExtensionProperties(
			window->gpu, NULL, &device_extension_count, device_extensions);
		assert(!err);

		for (uint32_t i = 0; i < device_extension_count; i++) {
			if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				device_extensions[i].extensionName)) {
				swapchainExtFound = 1;
				window->extension_names[window->enabled_extension_count++] =
					VK_KHR_SWAPCHAIN_EXTENSION_NAME;
			}
			assert(window->enabled_extension_count < 64);
		}

		free(device_extensions);
	}

	if (!swapchainExtFound) {
		ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find "
			"the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
			" extension.\n\nDo you have a compatible "
			"Vulkan installable client driver (ICD) installed?\n",
			"vkCreateInstance Failure");
	}

	if (window->validate) {
		window->CreateDebugReportCallback =
			(PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
			window->inst, "vkCreateDebugReportCallbackEXT");
		if (!window->CreateDebugReportCallback) {
			ERR_EXIT(
				"GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n",
				"vkGetProcAddr Failure");
		}
		VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
		dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		dbgCreateInfo.flags =
			VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		dbgCreateInfo.pfnCallback = dbgFunc;
		dbgCreateInfo.pUserData = NULL;
		dbgCreateInfo.pNext = NULL;
		err = window->CreateDebugReportCallback(window->inst, &dbgCreateInfo, NULL,
			&window->msg_callback);
		switch (err) {
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			ERR_EXIT("CreateDebugReportCallback: out of host memory\n",
				"CreateDebugReportCallback Failure");
			break;
		default:
			ERR_EXIT("CreateDebugReportCallback: unknown failure\n",
				"CreateDebugReportCallback Failure");
			break;
		}
	}

	// Having these GIPA queries of device extension entry points both
	// BEFORE and AFTER vkCreateDevice is a good test for the loader
	GET_INSTANCE_PROC_ADDR(window->inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
	GET_INSTANCE_PROC_ADDR(window->inst, GetPhysicalDeviceSurfaceFormatsKHR);
	GET_INSTANCE_PROC_ADDR(window->inst, GetPhysicalDeviceSurfacePresentModesKHR);
	GET_INSTANCE_PROC_ADDR(window->inst, GetPhysicalDeviceSurfaceSupportKHR);
	GET_INSTANCE_PROC_ADDR(window->inst, CreateSwapchainKHR);
	GET_INSTANCE_PROC_ADDR(window->inst, DestroySwapchainKHR);
	GET_INSTANCE_PROC_ADDR(window->inst, GetSwapchainImagesKHR);
	GET_INSTANCE_PROC_ADDR(window->inst, AcquireNextImageKHR);
	GET_INSTANCE_PROC_ADDR(window->inst, QueuePresentKHR);

	vkGetPhysicalDeviceProperties(window->gpu, &window->gpu_props);

	// Query with NULL data to get count
	vkGetPhysicalDeviceQueueFamilyProperties(window->gpu, &window->queue_count,
		NULL);

	window->queue_props = (VkQueueFamilyProperties *)malloc(
		window->queue_count * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(window->gpu, &window->queue_count,
		window->queue_props);
	assert(window->queue_count >= 1);

	// Graphics queue and MemMgr queue can be separate.
	// TODO: Add support for separate queues, including synchronization,
	//       and appropriate tracking for QueueSubmit
}

/*
 * Return 1 (true) if all layer names specified in check_names
 * can be found in given layer properties.
 */
static VkBool32 check_layers(uint32_t check_count, char **check_names,
	uint32_t layer_count,
	VkLayerProperties *layers) {
	for (uint32_t i = 0; i < check_count; i++) {
		VkBool32 found = 0;
		for (uint32_t j = 0; j < layer_count; j++) {
			if (!strcmp(check_names[i], layers[j].layerName)) {
				found = 1;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
			return 0;
		}
	}
	return 1;
}

VKAPI_ATTR void *VKAPI_CALL myrealloc(void *pUserData, void *pOriginal,
	size_t size, size_t alignment,
	VkSystemAllocationScope allocationScope) {
	return realloc(pOriginal, size);
}

VKAPI_ATTR void *VKAPI_CALL myalloc(void *pUserData, size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope) {
#ifdef _MSC_VER
	return _aligned_malloc(size, alignment);
#else
	return aligned_alloc(alignment, size);
#endif
}

VKAPI_ATTR void VKAPI_CALL myfree(void *pUserData, void *pMemory) {
#ifdef _MSC_VER
	_aligned_free(pMemory);
#else
	free(pMemory);
#endif
}

VKAPI_ATTR VkBool32 VKAPI_CALL
dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
uint64_t srcObject, size_t location, int32_t msgCode,
const char *pLayerPrefix, const char *pMsg, void *pUserData) {
	char *message = (char *)malloc(strlen(pMsg) + 100);

	assert(message);

	if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		sprintf(message, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode,
			pMsg);
	}
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		sprintf(message, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode,
			pMsg);
	}
	else {
		return false;
	}

#ifdef _WIN32
	MessageBox(NULL, message, "Alert", MB_OK);
#else
	printf("%s\n", message);
	fflush(stdout);
#endif
	free(message);

	/*
	* false indicates that layer should not bail-out of an
	* API call that had validation failures. This may mean that the
	* app dies inside the driver due to invalid parameter(s).
	* That's what would happen without validation layers, so we'll
	* keep that behavior here.
	*/
	return false;
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
		//.imageFormat = 
	};
}

