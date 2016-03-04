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

#ifdef _WIN32
static void run(DG_Window *window) {
	if (!window->prepared)
		return;
	draw(window);

	if (window->depthStencil > 0.99f)
		window->depthIncrement = -0.001f;
	if (window->depthStencil < 0.8f)
		window->depthIncrement = 0.001f;

	window->depthStencil += window->depthIncrement;
}

// On MS-Windows, make this a global, so it's available to WndProc()
DG_Window window;

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char tmp_str[] = "Vulkan Testing Practice Project";

	switch (uMsg) {
	case WM_CREATE:
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		if (window.prepared) {
			run(&window);
			break;
		}
	case WM_SIZE:
		window.width = lParam & 0xffff;
		window.height = lParam & 0xffff0000 >> 16;
		resize(&window);
		break;
	default:
		break;
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

static void create_window(DG_Window *window) {
	WNDCLASSEX win_class;

	// Initialize the window class structure:
	win_class.cbSize = sizeof(WNDCLASSEX);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = WndProc;
	win_class.cbClsExtra = 0;
	win_class.cbWndExtra = 0;
	win_class.hInstance = window->connection; // hInstance
	win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class.lpszMenuName = NULL;
	win_class.lpszClassName = window->name;
	win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	// Register window class:
	if (!RegisterClassEx(&win_class)) {
		// It didn't work, so try to give a useful error:
		printf("Unexpected error trying to start the application!\n");
		fflush(stdout);
		exit(1);
	}
	// Create window with the registered class:
	RECT wr = { 0, 0, window->width, window->height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	window->window = CreateWindowEx(0,
		window->name,           // class name
		window->name,           // app name
		WS_OVERLAPPEDWINDOW | // window style
		WS_VISIBLE | WS_SYSMENU,
		100, 100,           // x/y coords
		wr.right - wr.left, // width
		wr.bottom - wr.top, // height
		NULL,               // handle to parent
		NULL,               // handle to menu
		window->connection,   // hInstance
		NULL);              // no extra parameters
	if (!window->window) {
		// It didn't work, so try to give a useful error:
		printf("Cannot create a window in which to draw!\n");
		fflush(stdout);
		exit(1);
	}
}
#else  // _WIN32

static void handle_event(DG_Window *window,
	const xcb_generic_event_t *event) {
	switch (event->response_type & 0x7f) {
	case XCB_EXPOSE:
		draw(window);
		break;
	case XCB_CLIENT_MESSAGE:
		if ((*(xcb_client_message_event_t *)event).data.data32[0] ==
			(*window->atom_wm_delete_window).atom) {
			window->quit = true;
		}
		break;
	case XCB_KEY_RELEASE: {
		const xcb_key_release_event_t *key =
			(const xcb_key_release_event_t *)event;

		if (key->detail == 0x9)
			window->quit = true;
	} break;
	case XCB_DESTROY_NOTIFY:
		window->quit = true;
		break;
	case XCB_CONFIGURE_NOTIFY: {
		const xcb_configure_notify_event_t *cfg =
			(const xcb_configure_notify_event_t *)event;
		if ((window->width != cfg->width) || (window->height != cfg->height)) {
			window->width = cfg->width;
			window->height = cfg->height;
			resize(window);
		}
	} break;
	default:
		break;
	}
}

static void run(DG_Window *window) {
	xcb_flush(window->connection);

	while (!window->quit) {
		xcb_generic_event_t *event;

		event = xcb_poll_for_event(window->connection);
		if (event) {
			handle_event(window, event);
			free(event);
		}

		window_draw(window);

		if (window->depthStencil > 0.99f)
			window->depthIncrement = -0.001f;
		if (window->depthStencil < 0.8f)
			window->depthIncrement = 0.001f;

		window->depthStencil += window->depthIncrement;

		// Wait for work to finish before updating MVP.
		vkDeviceWaitIdle(window->device);
	}
}

static void create_window(DG_Window *window) {
	uint32_t value_mask, value_list[32];

	window->window = xcb_generate_id(window->connection);

	value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value_list[0] = window->screen->black_pixel;
	value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	xcb_create_window(window->connection, XCB_COPY_FROM_PARENT, window->window,
		window->screen->root, 0, 0, window->width, window->height, 0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT, window->screen->root_visual,
		value_mask, value_list);

	/* Magic code that will send notification when window is destroyed */
	xcb_intern_atom_cookie_t cookie =
		xcb_intern_atom(window->connection, 1, 12, "WM_PROTOCOLS");
	xcb_intern_atom_reply_t *reply =
		xcb_intern_atom_reply(window->connection, cookie, 0);

	xcb_intern_atom_cookie_t cookie2 =
		xcb_intern_atom(window->connection, 0, 16, "WM_DELETE_WINDOW");
	window->atom_wm_delete_window =
		xcb_intern_atom_reply(window->connection, cookie2, 0);

	xcb_change_property(window->connection, XCB_PROP_MODE_REPLACE, window->window,
		(*reply).atom, 4, 32, 1,
		&(*window->atom_wm_delete_window).atom);
	free(reply);

	xcb_map_window(window->connection, window->window);
}
#endif // _WIN32

static void init_vk_swapchain(DG_Window *window) {
	VkResult err;
	uint32_t i;

	// Create a WSI surface for the window:
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.hinstance = window->connection;
	createInfo.hwnd = window->window;

	err =
		vkCreateWin32SurfaceKHR(window->inst, &createInfo, NULL, &window->surface);

#else  // _WIN32
	VkXcbSurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.connection = window->connection;
	createInfo.window = window->window;

	err = vkCreateXcbSurfaceKHR(window->inst, &createInfo, NULL, &window->surface);
#endif // _WIN32

	// Iterate over each queue to learn whether it supports presenting:
	VkBool32 *supportsPresent =
		(VkBool32 *)malloc(window->queue_count * sizeof(VkBool32));
	for (i = 0; i < window->queue_count; i++) {
		window->fpGetPhysicalDeviceSurfaceSupportKHR(window->gpu, i, window->surface,
			&supportsPresent[i]);
	}

	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t presentQueueNodeIndex = UINT32_MAX;
	for (i = 0; i < window->queue_count; i++) {
		if ((window->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
			if (graphicsQueueNodeIndex == UINT32_MAX) {
				graphicsQueueNodeIndex = i;
			}

			if (supportsPresent[i] == VK_TRUE) {
				graphicsQueueNodeIndex = i;
				presentQueueNodeIndex = i;
				break;
			}
		}
	}
	if (presentQueueNodeIndex == UINT32_MAX) {
		// If didn't find a queue that supports both graphics and present, then
		// find a separate present queue.
		for (uint32_t i = 0; i < window->queue_count; ++i) {
			if (supportsPresent[i] == VK_TRUE) {
				presentQueueNodeIndex = i;
				break;
			}
		}
	}
	free(supportsPresent);

	// Generate error if could not find both a graphics and a present queue
	if (graphicsQueueNodeIndex == UINT32_MAX ||
		presentQueueNodeIndex == UINT32_MAX) {
		ERR_EXIT("Could not find a graphics and a present queue\n",
			"Swapchain Initialization Failure");
	}

	// TODO: Add support for separate queues, including presentation,
	//       synchronization, and appropriate tracking for QueueSubmit.
	// NOTE: While it is possible for an application to use a separate graphics
	//       and a present queues, this program assumes it is only using
	//       one:
	if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
		ERR_EXIT("Could not find a common graphics and a present queue\n",
			"Swapchain Initialization Failure");
	}

	window->graphics_queue_node_index = graphicsQueueNodeIndex;

	init_device(window);

	vkGetDeviceQueue(window->device, window->graphics_queue_node_index, 0,
		&window->queue);

	// Get the list of VkFormat's that are supported:
	uint32_t formatCount;
	err = window->fpGetPhysicalDeviceSurfaceFormatsKHR(window->gpu, window->surface,
		&formatCount, NULL);
	assert(!err);
	VkSurfaceFormatKHR *surfFormats =
		(VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
	err = window->fpGetPhysicalDeviceSurfaceFormatsKHR(window->gpu, window->surface,
		&formatCount, surfFormats);
	assert(!err);
	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
		window->format = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else {
		assert(formatCount >= 1);
		window->format = surfFormats[0].format;
	}
	window->color_space = surfFormats[0].colorSpace;

	// Get Memory information and properties
	vkGetPhysicalDeviceMemoryProperties(window->gpu, &window->memory_properties);
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

