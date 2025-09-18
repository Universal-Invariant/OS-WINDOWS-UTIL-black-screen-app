//
// Created by spoil on 04/14/2024.
//
#pragma once
#ifndef WINDOWINITIATOR_HPP
#define WINDOWINITIATOR_HPP

#include <string>
#include <windows.h>
#include <vector>
#include <iostream>





struct MonitorData {
    HMONITOR hMonitor;
    RECT rect;
    int index;
};

// Declare as extern — define in .cpp
extern std::vector<MonitorData> g_monitorList;
extern std::vector<HWND> g_windowHandles;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);


class WindowInitiator {
public:
    static std::string m_color;
    static bool disableKeyExit;
    std::vector<int> m_monitorIndices; // Now supports multiple monitors

    explicit WindowInitiator(std::string color, const bool& disableKeyExit, std::vector<int> monitorIndices = { -1 });
    void createWindow();
};

#endif // WINDOWINITIATOR_HPP