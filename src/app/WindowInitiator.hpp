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
#include <debugapi.h>
#include <winuser.h>
#include <wingdi.h>
#include <setupapi.h>
#include <devguid.h>

#include "MonitorDetection.hpp"



// Declare as extern � define in .cpp
extern std::vector<HWND> g_windowHandles;



class WindowInitiator {
public:
    static std::string m_color;
    static bool disableKeyExit;
    std::vector<int> m_monitorIndices;      // For -m
    std::vector<std::string> m_monitorPatterns; // For -M
    

    explicit WindowInitiator(std::string color, const bool& disableKeyExit,
        std::vector<int> monitorIndices = { -1 },
        std::vector<std::string> monitorPatterns = {});
    void createWindow();
};


#endif // WINDOWINITIATOR_HPP