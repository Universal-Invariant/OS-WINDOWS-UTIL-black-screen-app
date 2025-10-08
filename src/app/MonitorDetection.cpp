#include "MonitorDetection.hpp"
#include <setupapi.h>
#include <algorithm>
#include <cctype>

// Get friendly monitor name from target
std::string GetFriendlyNameFromTarget(LUID adapterId, UINT32 targetId) {
    DISPLAYCONFIG_TARGET_DEVICE_NAME deviceName = { };
    DISPLAYCONFIG_DEVICE_INFO_HEADER header = { };

    header.size = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);
    header.adapterId = adapterId;
    header.id = targetId;
    header.type = static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME);
    deviceName.header = header;

    LONG result = DisplayConfigGetDeviceInfo(&deviceName.header);
    if (result == ERROR_SUCCESS) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, deviceName.monitorFriendlyDeviceName, -1, nullptr, 0, nullptr, nullptr);
        if (size_needed > 0) {
            std::string name_utf8(size_needed - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, deviceName.monitorFriendlyDeviceName, -1, &name_utf8[0], size_needed, nullptr, nullptr);
            return name_utf8;
        }
    }
    return "Unknown Monitor";
}

// Helper function to compare LUIDs
bool operator==(const LUID& a, const LUID& b) {
    return a.LowPart == b.LowPart && a.HighPart == b.HighPart;
}

std::vector<MonitorData> EnumerateMonitorsWithNames() {
    std::vector<MonitorData> result;

    // Step 1: Get monitor names using DisplayConfig
    std::vector<std::string> friendlyNames;
    std::vector<POINT> configPositions;

    UINT32 num_paths = 0, num_modes = 0;
    LONG result_code = GetDisplayConfigBufferSizes(QDC_ALL_PATHS, &num_paths, &num_modes);
    if (result_code == ERROR_SUCCESS) {
        std::vector<DISPLAYCONFIG_PATH_INFO> paths(num_paths);
        std::vector<DISPLAYCONFIG_MODE_INFO> modes(num_modes);

        result_code = QueryDisplayConfig(QDC_ALL_PATHS, &num_paths, paths.data(), &num_modes, modes.data(), nullptr);
        if (result_code == ERROR_SUCCESS) {
            // Collect friendly names and positions for active targets
            for (UINT32 i = 0; i < num_paths; i++) {
                if (paths[i].flags & DISPLAYCONFIG_PATH_ACTIVE) {
                    auto& targetInfo = paths[i].targetInfo;
                    auto& sourceInfo = paths[i].sourceInfo;  // Use this!

                    // Get name
                    std::string name = GetFriendlyNameFromTarget(targetInfo.adapterId, targetInfo.id);
                    friendlyNames.push_back(name);

                    // Get source position using the sourceInfo from path
                    DISPLAYCONFIG_SOURCE_MODE* sourceMode = nullptr;
                    if (sourceInfo.modeInfoIdx < num_modes &&
                        modes[sourceInfo.modeInfoIdx].infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE &&
                        modes[sourceInfo.modeInfoIdx].adapterId == targetInfo.adapterId) {
                        sourceMode = &modes[sourceInfo.modeInfoIdx].sourceMode;
                    }

                    if (sourceMode) {
                        POINT pos = {
                            static_cast<LONG>(sourceMode->position.x),
                            static_cast<LONG>(sourceMode->position.y)
                        };
                        configPositions.push_back(pos);
                    }
                    else {
                        // Fallback - try to find matching source mode
                        for (UINT32 j = 0; j < num_modes; j++) {
                            if (modes[j].infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE &&
                                modes[j].adapterId == targetInfo.adapterId &&
                                modes[j].id == sourceInfo.id) {  // Match by ID, not by modeInfoIdx
                                sourceMode = &modes[j].sourceMode;
                                break;
                            }
                        }

                        if (sourceMode) {
                            POINT pos = {
                                static_cast<LONG>(sourceMode->position.x),
                                static_cast<LONG>(sourceMode->position.y)
                            };
                            configPositions.push_back(pos);
                        }
                        else {
                            // Final fallback - use dummy position (this should be rare)
                            configPositions.push_back({ 0, 0 });
                        }
                    }
                }
            }
        }
    }

    // Step 2: Get actual active monitors with EnumDisplayMonitors
    struct MatchData {
        const std::vector<std::string>* names;
        const std::vector<POINT>* positions;
        std::vector<MonitorData>* results;
    };

    MatchData data = { &friendlyNames, &configPositions, &result };

    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
        MONITORINFO mi = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(hMonitor, &mi)) {
            MatchData* data = reinterpret_cast<MatchData*>(dwData);

            std::string monitorName = "Monitor " + std::to_string(data->results->size() + 1);

            // Match by top-left position
            POINT monitorPos = { mi.rcMonitor.left, mi.rcMonitor.top };

            for (size_t i = 0; i < data->positions->size(); i++) {
                if ((*data->positions)[i].x == monitorPos.x && (*data->positions)[i].y == monitorPos.y) {
                    if (i < data->names->size()) {
                        monitorName = (*data->names)[i];
                        break;
                    }
                }
            }

            data->results->push_back({ hMonitor, mi.rcMonitor, static_cast<int>(data->results->size()), monitorName });
        }
        return TRUE;
        }, reinterpret_cast<LPARAM>(&data));

    return result;
}

// Helper function for case-insensitive comparison
std::string ToLower(const std::string& str) {
    std::string result = str;
    for (auto& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

// Find monitors by name pattern
std::vector<MonitorData> FindMonitorsByName(const std::vector<MonitorData>& allMonitors, const std::string& pattern) {
    std::vector<MonitorData> result;
    std::string patternLower = ToLower(pattern);

    for (const auto& monitor : allMonitors) {
        std::string monitorNameLower = ToLower(monitor.name);

        if (monitorNameLower.find(patternLower) != std::string::npos) {
            result.push_back(monitor);
        }
    }
    return result;
}

// Find monitor by index
MonitorData* FindMonitorByIndex(std::vector<MonitorData>& allMonitors, int index) {
    if (index >= 0 && index < static_cast<int>(allMonitors.size())) {
        return &allMonitors[index];
    }
    return nullptr;
}