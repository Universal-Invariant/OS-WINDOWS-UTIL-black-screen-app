
#pragma once
#ifndef MONITORDETECTION_HPP
#define MONITORDETECTION_HPP

#include <vector>
#include <string>
#include <windows.h>
#include <winuser.h>
#include <wingdi.h>

// Structure to hold matched monitor information
struct MonitorData {
    HMONITOR hMonitor;
    RECT rect;
    int index;
    std::string name;
};




std::string GetFriendlyNameFromTarget(LUID adapterId, UINT32 targetId);
std::vector<MonitorData> EnumerateMonitorsWithNames();
std::vector<MonitorData> FindMonitorsByName(const std::vector<MonitorData>& allMonitors, const std::string& pattern);
MonitorData* FindMonitorByIndex(std::vector<MonitorData>& allMonitors, int index);

#endif

/*
#pragma once
#ifndef MONITORDETECTION_HPP
#define MONITORDETECTION_HPP


#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601  // Windows 7 minimum


#include <vector>
#include <string>
#include <windows.h>
#include <winuser.h>
#include <wingdi.h>

// Required for DisplayConfig API
#include <setupapi.h>
#include <algorithm>
#include <cctype>

// Required for DisplayConfig API
#include <windef.h>

// Define missing types if not available
#ifndef DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE
#define DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE 3
#endif

// Structure to hold matched monitor information
struct MatchedMonitorInfo {
    HMONITOR hMonitor;
    RECT rect;
    int index;
    std::string name;
    std::string friendlyname;
};


std::string GetFriendlyNameFromTarget(LUID adapterId, UINT32 targetId);
std::vector<MatchedMonitorInfo> EnumerateMonitorsWithNames();
std::vector<MatchedMonitorInfo> FindMonitorsByName(const std::vector<MatchedMonitorInfo>& allMonitors, const std::string& pattern);
MatchedMonitorInfo* FindMonitorByIndex(std::vector<MatchedMonitorInfo>& allMonitors, int index);


#endif 

*/