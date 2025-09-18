#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#define WINVER 0x0601

#include <windows.h>
#include <windowsx.h>
#include "resource.h" 
#include <string>

// Structure to pass text and desired margins
struct DialogParams {
    const wchar_t* text;
    int marginX;    // horizontal margin
    int marginY;    // vertical margin
    int minWidth;   // minimum dialog width
    int minHeight;  // minimum dialog height
};

INT_PTR CALLBACK TextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        DialogParams* params = reinterpret_cast<DialogParams*>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(params));

        // Set static text
        HWND hStatic = GetDlgItem(hDlg, IDC_TEXT_STATIC);
        SetWindowTextW(hStatic, params->text);

        // Create and set monospaced font
        HFONT hFont = CreateFontW(
            -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FF_DONTCARE, L"Consolas"
        );
        if (!hFont) {
            hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, FF_MODERN, L"Courier New");
        }
        SendMessageW(hStatic, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SetWindowLongPtr(hStatic, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hFont));

        // Select font into DC to measure text
        HDC hdc = GetDC(hStatic);
        HFONT hOldFont = reinterpret_cast<HFONT>(SelectObject(hdc, hFont));

        // Split text into lines
        std::wstring text = params->text;
        int maxWidth = 0;
        int lineCount = 0;
        size_t start = 0, end = 0;

        while ((end = text.find_first_of(L"\n", start)) != std::wstring::npos) {
            std::wstring line = text.substr(start, end - start);
            SIZE size;
            if (GetTextExtentPoint32W(hdc, line.c_str(), static_cast<int>(line.length()), &size)) {
                maxWidth = max(maxWidth, size.cx);
            }
            lineCount++;
            start = end + 1;
        }
        // Last line
        if (start < text.length()) {
            std::wstring line = text.substr(start);
            SIZE size;
            if (GetTextExtentPoint32W(hdc, line.c_str(), static_cast<int>(line.length()), &size)) {
                maxWidth = max(maxWidth, size.cx);
            }
            lineCount++;
        }

        // Restore DC
        SelectObject(hdc, hOldFont);
        ReleaseDC(hStatic, hdc);

        // Calculate required size
        TEXTMETRICW tm;
        hdc = GetDC(hStatic);
        SelectObject(hdc, hFont);
        GetTextMetricsW(hdc, &tm);
        int lineHeight = tm.tmHeight + tm.tmExternalLeading;
        SelectObject(hdc, hOldFont);
        ReleaseDC(hStatic, hdc);

        int textHeight = lineCount * lineHeight;
        int dialogWidth = maxWidth + 2 * params->marginX + 15;
        int dialogHeight = textHeight + 2 * params->marginY + 75; // +50 for button and spacing

        // Enforce minimum size
        dialogWidth = max(dialogWidth, params->minWidth);
        dialogHeight = max(dialogHeight, params->minHeight);

        // Resize dialog
        RECT rc;
        GetWindowRect(hDlg, &rc);
        SetWindowPos(hDlg, nullptr, 0, 0, dialogWidth, dialogHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        {
            // Resize static control
            HWND hStatic = GetDlgItem(hDlg, IDC_TEXT_STATIC);
            SetWindowPos(hStatic, nullptr, params->marginX, params->marginY,
                dialogWidth - 2 * params->marginX, textHeight,
                SWP_NOZORDER | SWP_NOACTIVATE);

            // Reposition OK button
            HWND hButton = GetDlgItem(hDlg, IDOK);
            SetWindowPos(hButton, nullptr,
                (dialogWidth - 50) / 2,
                params->marginY + textHeight + 10,
                50, 24,
                SWP_NOZORDER | SWP_NOACTIVATE);

            // Center dialog on screen
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            SetWindowPos(hDlg, nullptr,
                (screenWidth - dialogWidth) / 2,
                (screenHeight - dialogHeight) / 2,
                0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        return TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;

    case WM_DESTROY: {
        HWND hStatic = GetDlgItem(hDlg, IDC_TEXT_STATIC);
        HFONT hFont = reinterpret_cast<HFONT>(GetWindowLongPtr(hStatic, GWLP_USERDATA));
        if (hFont) {
            DeleteObject(hFont);
        }
        break;
    }
    }
    return FALSE;
}

void ShowCustomTextDialog(const wchar_t* title, const wchar_t* text, int minWidth = 300, int minHeight = 200) {
    DialogParams params = {
        .text = text,
        .marginX = 20,
        .marginY = 20,
        .minWidth = minWidth,
        .minHeight = minHeight
    };

    DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_TEXT_DIALOG),
        nullptr,
        TextDialogProc,
        reinterpret_cast<LPARAM>(&params)
    );
}