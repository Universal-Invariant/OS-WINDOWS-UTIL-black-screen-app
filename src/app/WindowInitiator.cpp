//
// Created by spoil on 04/14/2024.
//

#include "WindowInitiator.hpp"

#include <algorithm>
#include <regex>
#include <utility>
#include <ranges>
#include <algorithm>
#include <iostream>
#include <iterator>


#include "ColorHandler.hpp"

LRESULT CALLBACK HandleWindowMessages(HWND windowHandle, UINT messageType, WPARAM windowParameterValue, LPARAM messageData);

std::vector<MonitorData> g_monitorList;
std::vector<HWND> g_windowHandles;

std::string WindowInitiator::m_color = "black";
bool WindowInitiator::disableKeyExit = false;


WindowInitiator::WindowInitiator(std::string color, const bool& disableKeyExit, std::vector<int> monitorIndices)
    : m_monitorIndices(std::move(monitorIndices)) {
    WindowInitiator::disableKeyExit = disableKeyExit;

    std::ranges::transform(color, color.begin(),
        [](const unsigned char stringCharacter) { return std::tolower(stringCharacter); });

    m_color = color == "black" ? std::move(color) :
        ColorHandler::colorMap.contains(color) ? ColorHandler::colorMap[color] : std::move(color);
}





BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MONITORINFO mi = { sizeof(MONITORINFO) };
    if (GetMonitorInfo(hMonitor, &mi)) {
        g_monitorList.push_back({ hMonitor, mi.rcMonitor, static_cast<int>(g_monitorList.size()) });
    }
    return TRUE;
}


void WindowInitiator::createWindow() {
    // Enumerate all monitors
    g_monitorList.clear();
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0);

    if (g_monitorList.empty()) {
        MessageBox(nullptr, "No monitors detected.", "Error", MB_ICONERROR);
        return;
    }

    // Validate monitor indices
    for (int idx : m_monitorIndices) {
        if (idx != -1 && (idx < 0 || idx >= static_cast<int>(g_monitorList.size()))) {
            char msg[256];
            sprintf_s(msg, "Invalid monitor index: %d", idx);
            MessageBox(nullptr, msg, "Error", MB_ICONERROR);
            return;
        }
    }

    const WNDCLASS windowClass = {
        .lpfnWndProc = HandleWindowMessages,
        .hInstance = GetModuleHandle(nullptr),
        .lpszClassName = "BlackWindowClass"
    };

    if (!GetClassInfo(GetModuleHandle(nullptr), "BlackWindowClass", const_cast<WNDCLASS*>(&windowClass))) {
        RegisterClass(&windowClass);
    }

    ShowCursor(FALSE);

    // Determine which monitors to cover
    std::vector<MonitorData> targetMonitors;

    if (m_monitorIndices.size() == 1 && m_monitorIndices[0] == -1) {
        // Cover all monitors
        targetMonitors = g_monitorList;
    }
    else {
        // Cover specified monitors
        for (int idx : m_monitorIndices) {
            if (idx == -1) {
                // If -1 is included, add all monitors
                for (const auto& m : g_monitorList) {
                    targetMonitors.push_back(m);
                }
            }
            else {
                targetMonitors.push_back(g_monitorList[idx]);
            }
        }
    }

    // Remove duplicates if needed (optional)
    std::sort(targetMonitors.begin(), targetMonitors.end(), [](const MonitorData& a, const MonitorData& b) {
        return a.index < b.index;
        });
    targetMonitors.erase(std::unique(targetMonitors.begin(), targetMonitors.end(), [](const MonitorData& a, const MonitorData& b) {
        return a.index == b.index;
        }), targetMonitors.end());

    // Create a window for each target monitor
    for (const auto& monitor : targetMonitors) {
        const auto windowHandle = CreateWindowEx(
            0,
            "BlackWindowClass",
            "Black Screen Application",
            WS_POPUP | WS_VISIBLE,
            monitor.rect.left,
            monitor.rect.top,
            monitor.rect.right - monitor.rect.left,
            monitor.rect.bottom - monitor.rect.top,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        if (windowHandle) {
            g_windowHandles.push_back(windowHandle);
            ShowWindow(windowHandle, SW_SHOW);
            UpdateWindow(windowHandle);
        }
    }

    // Message loop — shared across all windows
    MSG message;
    while (GetMessage(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);

        if (message.message == WM_QUIT) {
            break;
        }
    }

    // Cleanup
    for (auto hwnd : g_windowHandles) {
        DestroyWindow(hwnd);
    }
    g_windowHandles.clear();

    UnregisterClass("BlackWindowClass", GetModuleHandle(nullptr));
}


/*
void WindowInitiator::createWindow() {
    const WNDCLASS windowClass = {
        .lpfnWndProc = HandleWindowMessages,
        .hInstance = GetModuleHandle(nullptr),
        .lpszClassName = "BlackWindowClass"
    };
    RegisterClass(&windowClass);

    const auto windowHandle = CreateWindowEx(
        0,
        "BlackWindowClass",
        "Black Screen Application",
        WS_POPUP | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    ShowCursor(false);

    ShowWindow(windowHandle, SW_SHOW);
    UpdateWindow(windowHandle);

    MSG message;
    while (GetMessage(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);

        if (message.message == WM_QUIT) {
            break;
        }
    }

    UnregisterClass("BlackWindowClass", GetModuleHandle(nullptr));
}
*/

LRESULT CALLBACK HandleWindowMessages(const HWND windowHandle, const UINT messageType, const WPARAM windowParameterValue, const LPARAM messageData) { // NOLINT(*-misplaced-const)
    switch (messageType) {
        case WM_CLOSE:
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN: {
            if (!WindowInitiator::disableKeyExit) {
                PostQuitMessage(0);
            }
            return 0;
        }
        case WM_ERASEBKGND: {
            RECT clientRect;
            GetClientRect(windowHandle, &clientRect);
            HBRUSH colorBrush;

            if (WindowInitiator::m_color == "black") {
                colorBrush = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
            } else {
                if (std::regex_match(WindowInitiator::m_color, std::regex("^#([a-fA-F0-9]{6}|[a-fA-F0-9]{3})$"))) {
                    auto [red, green, blue] = ColorHandler::convertHextoRGB(WindowInitiator::m_color);
                    colorBrush = CreateSolidBrush(RGB(red, green, blue));
                } else {
                    const auto errorMessage = "Invalid color argument. Expected a hex color code or color name.";
                    MessageBox(nullptr, errorMessage, "Error - Black Screen Application", MB_ICONERROR | MB_OK);
                    throw std::invalid_argument(errorMessage);
                }
            }

            FillRect(reinterpret_cast<HDC>(windowParameterValue), &clientRect, colorBrush);

            if (WindowInitiator::m_color != "black") {
                DeleteObject(colorBrush);
            }

            return 1;
        }
        default:
            return DefWindowProc(windowHandle, messageType, windowParameterValue, messageData);
    }
}


