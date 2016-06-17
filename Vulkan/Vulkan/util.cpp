/* Copyright (C) 2016 Daniel Grimshaw
*
* util.cpp | General utilities to enhance the Vulkan API
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
#include "util.h"
#include "BUILD_OPTIONS.h"

#if BUILD_ENABLE_VULKAN_RUNTIME_DEBUG

void ErrorCheck(VkResult result) {
	using namespace std;
	if (result < 0) {
		switch (result) {
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
			break;
		case VK_ERROR_INITIALIZATION_FAILED:
			cout << "VK_ERROR_INITIALIZATION_FAILED" << endl;
			break;
		case VK_ERROR_DEVICE_LOST:
			cout << "VK_ERROR_DEVICE_LOST" << endl;
			break;
		case VK_ERROR_MEMORY_MAP_FAILED:
			cout << "VK_ERROR_MEMORY_MAP_FAILED" << endl;
			break;
		case VK_ERROR_LAYER_NOT_PRESENT:
			cout << "VK_ERROR_LAYER_NOT_PRESENT" << endl;
			break;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			cout << "VK_ERROR_EXTENSION_NOT_PRESENT" << endl;
			break;
		case VK_ERROR_FEATURE_NOT_PRESENT:
			cout << "VK_ERROR_FEATURE_NOT_PRESENT" << endl;
			break;
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			cout << "VK_ERROR_INCOMPATIBLE_DRIVER" << endl;
			break;
		case VK_ERROR_TOO_MANY_OBJECTS:
			cout << "VK_ERROR_TOO_MANY_OBJECTS" << endl;
			break;
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			cout << "VK_ERROR_FORMAT_NOT_SUPPORTED" << endl;
			break;
		case VK_ERROR_SURFACE_LOST_KHR:
			cout << "VK_ERROR_SURFACE_LOST_KHR" << endl;
			break;
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			cout << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << endl;
			break;
		case VK_SUBOPTIMAL_KHR:
			cout << "VK_SUBOPTIMAL_KHR" << endl;
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			cout << "VK_ERROR_OUT_OF_DATE_KHR" << endl;
			break;
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			cout << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" << endl;
			break;
		case VK_ERROR_VALIDATION_FAILED_EXT:
			cout << "VK_ERROR_VALIDATION_FAILED_EXT" << endl;
			break;
		case VK_ERROR_INVALID_SHADER_NV:
			cout << "VK_ERROR_INVALID_SHADER_NV" << endl;
			break;
		default:
			break;
		}

		assert(0 && "Vulkan runtime error");
	}
}

#else

void ErrorCheck(VkResult result) {};

#endif //BUILD_ENABLE_VULKAN_RUNTIME_DEBUG