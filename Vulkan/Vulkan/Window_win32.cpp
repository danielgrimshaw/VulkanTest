/* Copyright (C) 2016 Daniel Grimshaw
*
* Window_win32.cpp | Implementation of OS specific window functions for Win32 systems.
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

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Window.h"
#include "Renderer.h"
#include <assert.h>
#include <iostream>
#include <chrono>

#include <ctime>

#if VK_USE_PLATFORM_WIN32_KHR


LRESULT CALLBACK WindowsEventHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Window * window = reinterpret_cast<Window *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

	switch (uMsg) {
	case WM_CLOSE:
		window->close();
		return 0;
	case WM_SIZE:
		// Window has changed size
		// Should rebuild all resources
		// Resizing has been disabled
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

uint64_t Window::_win32_class_id_counter = 0;

void Window::_InitOSWindow() {
	WNDCLASSEX win_class {};
	assert(_surface_size_x > 0);
	assert(_surface_size_y > 0);

	_win32_instance = GetModuleHandle(nullptr);
	_win32_class_name = _window_name + "_" + std::to_string(_win32_class_id_counter);
	_win32_class_id_counter++;

	// Initialize window structure
	win_class.cbSize = sizeof(WNDCLASSEX);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = WindowsEventHandler;
	win_class.cbClsExtra = 0;
	win_class.cbWndExtra = 0;
	win_class.hInstance = _win32_instance; // hInstance
	win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class.lpszMenuName = NULL;
	win_class.lpszClassName = _win32_class_name.c_str();
	win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	// Register window class
	if (!RegisterClassEx(&win_class)) {
		// Failed to register window class
		assert(0 && "Cannot create a window in which to draw!\n");
		fflush(stdout);
		std::exit(-1);
	}

	DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	// Create window with the registered class
	RECT wr = { 0, 0, LONG(_surface_size_x), LONG(_surface_size_y) };
	AdjustWindowRectEx(&wr, style, FALSE, ex_style);
	_win32_window = CreateWindowEx(0,
		_win32_class_name.c_str(),   // class name
		_window_name.c_str(),        // app name
		style,                       // window style
		CW_USEDEFAULT, CW_USEDEFAULT,// x/y coords
		wr.right - wr.left,          // width
		wr.bottom - wr.top,          // height
		NULL,                        // handle to parent
		NULL,                        // handle to menu
		_win32_instance,             // hInstance
		NULL);                       // no extra parameters

	if (!_win32_window) {
		// Failure
		assert(0 && "Cannot create a window in which to draw!\n");
		fflush(stdout);
		std::exit(-1);
	}

	SetWindowLongPtr(_win32_window, GWLP_USERDATA, (LONG_PTR)this);

	ShowWindow(_win32_window, SW_SHOW);
	SetForegroundWindow(_win32_window);
	SetFocus(_win32_window);
}

void Window::_DeInitOSWindow() {
	DestroyWindow(_win32_window);
	UnregisterClass(_win32_class_name.c_str(), _win32_instance);
}

#if BUILD_ENABLE_FRAMERATE
std::clock_t last_time = std::clock();
#endif

void Window::_UpdateOSWindow() {
	MSG msg;
	if (PeekMessage(&msg, _win32_window, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#if BUILD_ENABLE_FRAMERATE //TODO
	std::clock_t diff = std::clock() - last_time;
	if (diff == 0)
		std::cout<<"1000+"<<std::endl;
	else
		std::cout<<1.0/(diff/1000.0)<<std::endl;
	last_time = std::clock();
#endif
}

void Window::_InitOSSurface() {
	VkWin32SurfaceCreateInfoKHR surface_create_info {};
	surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_create_info.hinstance = _win32_instance;
	surface_create_info.hwnd = _win32_window;
	vkCreateWin32SurfaceKHR(_renderer->getInstance(), &surface_create_info, nullptr, &_surface);
}
#endif