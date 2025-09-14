#include <windows.h>
#include <commctrl.h>
#include "prototypes.h"

// Variables externes nécessaires
extern HWND g_hMainWindow;
extern HHOOK g_hHook;

// Simple strchr implementation
char* simple_strchr(const char* str, int c) {
    while (*str) {
        if (*str == c) return (char*)str;
        str++;
    }
    return NULL;
}

// Conversion simple string vers entier
int simple_atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    // Skip whitespace
    while (*str == ' ' || *str == '\t') str++;
    
    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Convert digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

// Simple strstr implementation
char* simple_strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

// Hook procedure pour centrer les dialogues
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HCBT_ACTIVATE) {
        HWND hWnd = (HWND)wParam;
        char className[256];
        GetClassNameA(hWnd, className, sizeof(className));
        
        if (lstrcmpA(className, "#32770") == 0) { // Dialog class
            // Centrer la dialog sur la fenêtre principale
            RECT mainRect, dlgRect;
            GetWindowRect(g_hMainWindow, &mainRect);
            GetWindowRect(hWnd, &dlgRect);
            
            int centerX = mainRect.left + (mainRect.right - mainRect.left - (dlgRect.right - dlgRect.left)) / 2;
            int centerY = mainRect.top + (mainRect.bottom - mainRect.top - (dlgRect.bottom - dlgRect.top)) / 2;
            
            SetWindowPos(hWnd, NULL, centerX, centerY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            
            // Désinstaller le hook
            UnhookWindowsHookEx(g_hHook);
            g_hHook = NULL;
        }
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// MessageBox centrée sur la fenêtre principale
int ShowCenteredMessageBox(const char* text, const char* caption, UINT type) {
    // Installer le hook pour centrer la MessageBox
    g_hHook = SetWindowsHookEx(WH_CBT, CBTProc, NULL, GetCurrentThreadId());
    int result = MessageBoxA(g_hMainWindow, text, caption, type);
    
    // Nettoyer le hook si il est encore installé
    if (g_hHook) {
        UnhookWindowsHookEx(g_hHook);
        g_hHook = NULL;
    }
    
    return result;
}

// Fonction pour créer un effet de clignotement en cas d'erreur (style AmigaOS)
void FlashError(HWND hwnd) {
    if (!hwnd) return;
    
    // Faire clignoter 3 fois
    for (int i = 0; i < 3; i++) {
        // Inverser les couleurs (effet flash)
        HDC hdc = GetDC(hwnd);
        RECT rect;
        GetClientRect(hwnd, &rect);
        
        // Flash rouge
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 100, 100));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);
        ReleaseDC(hwnd, hdc);
        
        Sleep(80); // Court délai
        
        // Restaurer l'apparence normale
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        
        Sleep(80); // Court délai
    }
}

// Fonction pour créer toutes les fenêtres centrées
HWND CreateCenteredWindow(const char* className, const char* windowName, DWORD style, int width, int height, HWND parent, HINSTANCE hInstance) {
    // Obtenir la position de la fenêtre principale
    RECT mainRect;
    GetWindowRect(g_hMainWindow, &mainRect);
    int centerX = mainRect.left + (mainRect.right - mainRect.left - width) / 2;
    int centerY = mainRect.top + (mainRect.bottom - mainRect.top - height) / 2;
    
    return CreateWindowA(className, windowName, style, centerX, centerY, width, height, parent, NULL, hInstance, NULL);
}