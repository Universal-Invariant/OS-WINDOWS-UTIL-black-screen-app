#include "WindowInitiator.hpp"
#include <shellapi.h> // for CommandLineToArgvW


extern void ShowCustomTextDialog(const wchar_t* title, const wchar_t* text, int width = 300, int height = 200);



std::string wstring_to_string(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, nullptr, nullptr);
    return str;
}

std::wstring string_to_wstring(const std::string& str) {
    if (str.empty()) return {};
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}



void showHelp() {
    const wchar_t* helpText =
        L"Black Screen Application - Turn monitors black (emulate off)\n"
        L"\n"
        L"Usage:\n"
        L"  black_screen_app.exe [OPTIONS]\n"
        L"\n"
        L"Options:\n"
        L"  -m, --monitor <indices>     Specify monitor indices to turn off (1-based).\n"
        L"                              Examples: -m 1 2, -m 2,3,4, -m 0 (all)\n"
        L"  -M, --monitor-name <names>  Specify monitor names to turn off (substring match).\n"
        L"                              Examples: -M \"Dell\" \"HP\", -M \"Laptop\"\n"
        L"  -c, --color <color>         Background color (e.g., #FF0000).\n"
        L"  -dke, --disable-key-exit    Disable exiting with any key press.\n"
        L"  -l, --list                  List all detected monitors.\n"
        L"  -h, --help                  Show this help message.\n"
        L"\n"
        L"Examples:\n"
        L"  black_screen_app.exe -m 0        → All monitors\n"
        L"  black_screen_app.exe -m 1 2      → Monitors 1 and 2\n"
        L"  black_screen_app.exe -M \"Dell\"   → Monitor with 'Dell' in name\n"
        L"  black_screen_app.exe -M \"Dell\" \"HP\" → Multiple monitors by name\n"
        L"  black_screen_app.exe -c \"#00FF00\" → Green background\n";

    ShowCustomTextDialog(L"Help", helpText);
}

// Helper: split string by delimiters
std::vector<std::string> split(const std::string& s, const std::string& delims = " \t\n\r") {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;

    while ((end = s.find_first_of(delims, start)) != std::string::npos) {
        if (end != start) {
            tokens.push_back(s.substr(start, end - start));
        }
        start = end + 1;
    }

    if (start < s.length()) {
        tokens.push_back(s.substr(start));
    }

    return tokens;
}

extern std::vector<DISPLAY_DEVICEW> g_devices;
extern std::vector<MonitorData> g_monitors;
extern void EnumerateDisplayConfig();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    std::string backgroundColor = "black";
    bool shouldExitOnKeyPress = false;    
    std::vector<int> monitorIndices = { 0 }; // default: all (0-based internally)
    std::vector<std::string> monitorPatterns; // for -M
    


    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        MessageBoxW(nullptr, L"Failed to parse command line.", L"Error", MB_ICONERROR);
        return 1;
    }

    
    // Check for help or no args
    if (argc == 1) {
        showHelp();
        LocalFree(argv);
        return 0;
    }

    
    
    
    g_monitors.clear();    
    g_monitors = EnumerateMonitorsWithNames();

    

    
    // First pass: check for --list, --help
    for (int i = 1; i < argc; ++i) {
        std::wstring currentArgW = argv[i];
        std::string currentArg = wstring_to_string(currentArgW);
        

        if (currentArg == "-l" || currentArg == "--list") {
            
            
            std::wstring listText = L"Detected Monitors:\n";
            listText += L"====================\n\n";
            listText += L"Idx  Left    Top     Right   Bottom  Name\n";
            listText += L"---  ------  ------  ------  ------  ----\n";
            
            for (size_t idx = 0; idx < g_monitors.size(); ++idx) {
                auto& m = g_monitors[idx];
                wchar_t buffer[1256];
                std::wstring name(m.name.begin(), m.name.end());
                swprintf_s(buffer, L"%-3zu  %-6d  %-6d  %-6d  %-6d  (%d) %ls\n",
                    idx + 1,
                    m.rect.left,
                    m.rect.top,
                    m.rect.right,
                    m.rect.bottom,                    
                    m.index,
                    name.c_str());

                listText += buffer;
            }
               

            ShowCustomTextDialog(L"Monitor List", listText.c_str());
            LocalFree(argv);
            return 0;
        }
        else if (currentArg == "-h" || currentArg == "--help") {
            showHelp();
            LocalFree(argv);
            return 0;
        }
    }

    // Second pass: parse arguments
    for (int i = 1; i < argc; ++i) {
        std::wstring currentArgW = argv[i];
        std::string currentArg = wstring_to_string(currentArgW);

        if (currentArg == "-m" || currentArg == "--monitor") {
            if (i + 1 >= argc) {
                MessageBoxW(nullptr, L"Error: Missing value for --monitor", L"Error", MB_ICONERROR);
                LocalFree(argv);
                return 1;
            }

            monitorIndices.clear();

            // Consume all following tokens until next flag
            while (i + 1 < argc) {
                std::wstring nextArgW = argv[i + 1];
                std::string nextArg = wstring_to_string(nextArgW);

                if (!nextArg.empty() && nextArg[0] == '-') {
                    break;
                }

                std::vector<std::string> parts = split(nextArg, ",");
                for (const std::string& part : parts) {
                    size_t first = part.find_first_not_of(" \t");
                    size_t last = part.find_last_not_of(" \t");
                    if (first == std::string::npos) continue;
                    std::string token = part.substr(first, (last - first + 1));

                    try {
                        size_t pos;
                        int idx = std::stoi(token, &pos);
                        if (pos != token.size()) {
                            throw std::invalid_argument("Trailing characters");
                        }
                        monitorIndices.push_back(idx);
                    }
                    catch (...) {
                        std::wstring msg = L"Error: Invalid monitor index: '" + string_to_wstring(token) + L"'";
                        MessageBoxW(nullptr, msg.c_str(), L"Error", MB_ICONERROR);
                        LocalFree(argv);
                        return 1;
                    }
                }

                ++i;
            }

            if (monitorIndices.empty()) {
                MessageBoxW(nullptr, L"Error: No monitor indices provided after --monitor", L"Error", MB_ICONERROR);
                LocalFree(argv);
                return 1;
            }

            size_t monitorCount = g_monitors.size();

            std::vector<int> validatedIndices;
            bool useAll = true;

            for (int userIndex : monitorIndices) {
                if (userIndex == 0) {
                    // keep useAll = true
                }
                else if (userIndex >= 1 && userIndex <= static_cast<int>(monitorCount)) {
                    useAll = false;
                    validatedIndices.push_back(userIndex - 1);
                }
                else {
                    std::wstring msg = L"Error: Monitor index " + std::to_wstring(userIndex) +
                        L" is out of range. Valid: 1 to " + std::to_wstring(monitorCount) + L" (or 0 for all).";
                    MessageBoxW(nullptr, msg.c_str(), L"Error", MB_ICONERROR);
                    LocalFree(argv);
                    return 1;
                }
            }

            if (useAll) {
                monitorIndices = { -1 };
            }
            else {
                monitorIndices = validatedIndices;
            }

        }
        else if (currentArg == "-M" || currentArg == "--monitor-name") {
            if (!monitorIndices.empty() && monitorIndices.size() == 1 && monitorIndices[0] == 0) {
                // if default
                monitorIndices.clear();
            }
            if (!monitorIndices.empty()) {
                MessageBoxW(nullptr, L"Error: Cannot use both -m and -M", L"Error", MB_ICONERROR);
                LocalFree(argv);
                return 1;
            }
            

            // Parse string patterns
            monitorPatterns.clear();
            while (i + 1 < argc) {
                std::wstring nextArgW = argv[i + 1];
                std::string nextArg = wstring_to_string(nextArgW);
                if (!nextArg.empty() && nextArg[0] == '-') break;
                monitorPatterns.push_back(nextArg);
                ++i;
            }
            if (monitorPatterns.empty()) {
                MessageBoxW(nullptr, L"Error: No monitor names provided after -M", L"Error", MB_ICONERROR);
                LocalFree(argv);
                return 1;
            }
        }
        else if (currentArg == "-c" || currentArg == "--color") {
            if (i + 1 >= argc) {
                MessageBoxW(nullptr, L"Error: Missing value for --color", L"Error", MB_ICONERROR);
                LocalFree(argv);
                return 1;
            }
            backgroundColor = wstring_to_string(argv[++i]);

        }
        else if (currentArg == "-dke" || currentArg == "--disable-key-exit") {
            shouldExitOnKeyPress = true;

        }
        else {
            std::wstring msg = L"Error: Unknown argument: " + string_to_wstring(currentArg) + L"\nUse --help for usage.";
            MessageBoxW(nullptr, msg.c_str(), L"Error", MB_ICONERROR);
            LocalFree(argv);
            return 1;
        }
    }

    LocalFree(argv);

    // Launch the black screen windows    
    WindowInitiator windowInitiator(backgroundColor, shouldExitOnKeyPress, monitorIndices, monitorPatterns);
    windowInitiator.createWindow();

    return 0;
}