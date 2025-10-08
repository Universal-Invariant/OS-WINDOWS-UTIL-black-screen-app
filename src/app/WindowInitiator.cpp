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

std::vector<HWND> g_windowHandles;
std::vector<DISPLAY_DEVICEW> g_devices;
std::vector<MonitorData> g_monitors;

std::string WindowInitiator::m_color = "black";
bool WindowInitiator::disableKeyExit = false;


WindowInitiator::WindowInitiator(std::string color, const bool& disableKeyExit,
    std::vector<int> monitorIndices,
    std::vector<std::string> monitorPatterns)
    : m_monitorIndices(std::move(monitorIndices)),
    m_monitorPatterns(std::move(monitorPatterns))
    {
    WindowInitiator::disableKeyExit = disableKeyExit;

    std::ranges::transform(color, color.begin(),
        [](const unsigned char stringCharacter) { return std::tolower(stringCharacter); });

    m_color = color == "black" ? std::move(color) :
        ColorHandler::colorMap.contains(color) ? ColorHandler::colorMap[color] : std::move(color);
}







// Structure to store monitor info with adapter/target IDs
struct ExtendedMonitorInfo {
    HMONITOR hMonitor;
    RECT rect;
    int index;
    std::string name;
    LUID adapterId;
    UINT32 targetId;
};




std::vector<ExtendedMonitorInfo> g_extendedMonitorList;

void EnumerateDisplayConfig() {
    g_extendedMonitorList.clear();

    UINT32 num_paths = 0;
    UINT32 num_modes = 0;

    // Get required buffer sizes
    LONG result = GetDisplayConfigBufferSizes(QDC_ALL_PATHS, &num_paths, &num_modes);
    if (result != ERROR_SUCCESS) return;

    // Allocate buffers
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(num_paths);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(num_modes);

    // Query display config
    result = QueryDisplayConfig(QDC_ALL_PATHS, &num_paths, paths.data(), &num_modes, modes.data(), nullptr);
    if (result != ERROR_SUCCESS) return;

    // Collect target info (monitors)
    std::vector<std::pair<LUID, UINT32>> targets; // adapterId, targetId
    for (UINT32 i = 0; i < num_modes; i++) {
        if (modes[i].infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET) {
            targets.push_back({ modes[i].adapterId, modes[i].id });
        }
    }

    // Now enumerate actual monitors to get positions
    std::vector<RECT> monitorRects;
    std::vector<std::string> monitorNames;

    // Get friendly names for targets
    for (const auto& [adapterId, targetId] : targets) {
        std::string name = GetFriendlyNameFromTarget(adapterId, targetId);
        monitorNames.push_back(name);
    }

    // Now match with EnumDisplayMonitors
    g_monitors.clear();
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
        MONITORINFO mi = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(hMonitor, &mi)) {
            std::string name = "Monitor " + std::to_string(g_monitors.size() + 1);

            // Try to match with our collected names (simplified - you may need more complex matching)
            //if (g_monitorList.size() < monitorNames.size()) {
                //name = monitorNames[g_monitorList.size()];
            //}

            g_monitors.push_back({ hMonitor, mi.rcMonitor, static_cast<int>(g_monitors.size()), name });
        }
        return TRUE;
        }, 0);
}








// Helper function for case-insensitive comparison
extern std::string ToLower(const std::string& str);

void WindowInitiator::createWindow() {



    if (g_monitors.empty()) {
        MessageBox(nullptr, L"No monitors detected.", L"Error", MB_ICONERROR);
        return;
    }

    std::vector<MonitorData> targetMonitors;

    if (m_monitorPatterns.size() > 0) {
        // Handle string patterns (-M)
        if (m_monitorPatterns.size() == 1 && m_monitorPatterns[0] == "*") {
            // Use all monitors
            targetMonitors = g_monitors;
        }
        else {
            // Match patterns
            for (const std::string& pattern : m_monitorPatterns) {
                bool matched = false;
                for (const auto& monitor : g_monitors) {
                    std::string monitorNameLower = monitor.name;
                    std::string patternLower = pattern;
                    std::ranges::transform(monitorNameLower, monitorNameLower.begin(), ::tolower);
                    std::ranges::transform(patternLower, patternLower.begin(), ::tolower);                

                    if (monitorNameLower.find(patternLower) != std::string::npos) {
                        targetMonitors.push_back(monitor);
                        matched = true;
                        break; // greedy match
                    }
                }
                if (!matched) {
                    char msg[512];
                    sprintf_s(msg, "No monitor found matching pattern: '%s'", pattern.c_str());
                    MessageBoxA(nullptr, msg, "Warning", MB_ICONWARNING);
                }
            }
        }
    }
    else {
        // Handle numeric indices (-m) — existing logic
        if (m_monitorIndices.size() == 1 && m_monitorIndices[0] == -1) {
            // Use all monitors
            targetMonitors = g_monitors;
        }
        else {
            for (int idx : m_monitorIndices) {
                if (idx == -1) {
                    // If -1 is included, add all monitors
                    for (const auto& m : g_monitors) {
                        targetMonitors.push_back(m);
                    }
                }
                else {
                    if (idx >= 0 && idx < static_cast<int>(g_monitors.size())) {
                        targetMonitors.push_back(g_monitors[idx]);
                    }
                    else {
                        char msg[256];
                        sprintf_s(msg, "Invalid monitor index: %d", idx);
                        MessageBoxA(nullptr, msg, "Error", MB_ICONERROR);
                        return;
                    }
                }
            }
        }
    }

    // Remove duplicates
    std::sort(targetMonitors.begin(), targetMonitors.end(), [](const MonitorData& a, const MonitorData& b) {
        return a.index < b.index;
        });
    targetMonitors.erase(std::unique(targetMonitors.begin(), targetMonitors.end(), [](const MonitorData& a, const MonitorData& b) {
        return a.index == b.index;
        }), targetMonitors.end());

    // Create windows (same as before)
    const WNDCLASS windowClass = {
        .lpfnWndProc = HandleWindowMessages,
        .hInstance = GetModuleHandle(nullptr),
        .lpszClassName = L"BlackWindowClass"
    };

    if (!GetClassInfo(GetModuleHandle(nullptr), L"BlackWindowClass", const_cast<WNDCLASS*>(&windowClass))) {
        RegisterClass(&windowClass);
    }

    ShowCursor(FALSE);

    for (const auto& monitor : targetMonitors) {
        const auto windowHandle = CreateWindowEx(
            0,
            L"BlackWindowClass",
            L"Black Screen Application",
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

    // Message loop
    MSG message;
    while (GetMessage(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
        if (message.message == WM_QUIT) break;
    }

    // Cleanup
    for (auto hwnd : g_windowHandles) {
        DestroyWindow(hwnd);
    }
    g_windowHandles.clear();
    UnregisterClass(L"BlackWindowClass", GetModuleHandle(nullptr));
}
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
                    const auto errorMessage = std::wstring(L"Invalid color argument. Expected a hex color code or color name.");
                    MessageBox(nullptr, errorMessage.c_str(), L"Error - Black Screen Application", MB_ICONERROR | MB_OK);
                    throw std::invalid_argument("Invalid color argument. Expected a hex color code or color name.");
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


