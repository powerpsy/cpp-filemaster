// viewers.c - Viewers et éditeurs de fichiers (ASCII/HEX)
#include <windows.h>
#include <stdio.h>
#include "prototypes.h"

// Variables globales externes (définies dans main.c)
extern HWND g_hMainWindow;
extern HFONT g_hFontUTF8;
extern HFONT g_hFontMono;

// Structure pour les données du viewer/éditeur
typedef struct {
    char filePath[MAX_PATH];
    BYTE* fileData;
    DWORD fileSize;
    BOOL isEditor;
    BOOL isHex;
    BOOL isModified;
    HWND hEdit;
    HWND hSaveButton;
    HWND hCloseButton;
} ViewerData;

// Procédure de fenêtre pour les viewers/éditeurs
LRESULT CALLBACK ViewerWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ViewerData* data = (ViewerData*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_CREATE: {
            ViewerData* createData = (ViewerData*)((CREATESTRUCT*)lParam)->lpCreateParams;
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)createData);
            data = createData;
            
            // Créer le contrôle d'édition
            DWORD editStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL;
            if (!data->isEditor) editStyle |= ES_READONLY;
            
            data->hEdit = CreateWindowA("EDIT", "", editStyle,
                10, 10, 780, data->isEditor ? 500 : 510, hwnd, NULL, GetModuleHandleA(NULL), NULL);
            
            // Appliquer la police monospace
            if (g_hFontMono) {
                SendMessageA(data->hEdit, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
            }
            
            // Charger et afficher le contenu du fichier
            if (data->isHex) {
                char* hexText;
                FormatHexData(data->fileData, data->fileSize, &hexText);
                if (hexText) {
                    SetWindowTextA(data->hEdit, hexText);
                    HeapFree(GetProcessHeap(), 0, hexText);
                }
            } else {
                char* asciiText = FormatASCIIData(data->fileData, data->fileSize);
                if (asciiText) {
                    SetWindowTextA(data->hEdit, asciiText);
                    HeapFree(GetProcessHeap(), 0, asciiText);
                }
            }
            
            // Créer le bouton Sauvegarder si c'est un éditeur
            if (data->isEditor) {
                data->hSaveButton = CreateWindowA("BUTTON", "Sauvegarder", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    300, 520, 100, 30, hwnd, (HMENU)1001, GetModuleHandleA(NULL), NULL);
                if (g_hFontUTF8) {
                    SendMessageA(data->hSaveButton, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                }
                
                // Bouton Fermer pour éditeur
                data->hCloseButton = CreateWindowA("BUTTON", "Fermer", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    410, 520, 100, 30, hwnd, (HMENU)1002, GetModuleHandleA(NULL), NULL);
                if (g_hFontUTF8) {
                    SendMessageA(data->hCloseButton, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                }
            } else {
                // Bouton Fermer pour viewer (centré)
                data->hCloseButton = CreateWindowA("BUTTON", "Fermer", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    350, 530, 100, 30, hwnd, (HMENU)1002, GetModuleHandleA(NULL), NULL);
                if (g_hFontUTF8) {
                    SendMessageA(data->hCloseButton, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                }
            }
            
            return 0;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1001 && data && data->isEditor) { // Bouton Sauvegarder
                int textLen = GetWindowTextLengthA(data->hEdit);
                char* text = (char*)HeapAlloc(GetProcessHeap(), 0, textLen + 1);
                if (text) {
                    GetWindowTextA(data->hEdit, text, textLen + 1);
                    
                    if (data->isHex) {
                        // Pour l'éditeur HEX, on sauvegarde le texte tel quel (format HEX)
                        // L'utilisateur peut modifier le format HEX directement
                        if (SaveFileData(data->filePath, (BYTE*)text, lstrlenA(text))) {
                            data->isModified = FALSE;
                            ShowCenteredMessageBox("Fichier HEX sauvegardé avec succès", "Sauvegarde", MB_OK | MB_ICONINFORMATION);
                        } else {
                            ShowCenteredMessageBox("Erreur lors de la sauvegarde HEX", "Erreur", MB_OK | MB_ICONERROR);
                        }
                    } else {
                        if (SaveFileData(data->filePath, (BYTE*)text, lstrlenA(text))) {
                            data->isModified = FALSE;
                            ShowCenteredMessageBox("Fichier sauvegardé avec succès", "Sauvegarde", MB_OK | MB_ICONINFORMATION);
                        } else {
                            ShowCenteredMessageBox("Erreur lors de la sauvegarde", "Erreur", MB_OK | MB_ICONERROR);
                        }
                    }
                    HeapFree(GetProcessHeap(), 0, text);
                }
            } else if (LOWORD(wParam) == 1002) { // Bouton Fermer
                SendMessageA(hwnd, WM_CLOSE, 0, 0);
            } else if (HIWORD(wParam) == EN_CHANGE && data && data->isEditor) {
                // Le contenu de l'éditeur a changé
                data->isModified = TRUE;
            }
            break;
        }
        
        case WM_CLOSE: {
            if (data && data->isEditor && data->isModified) {
                int result = ShowCenteredMessageBox("Le fichier a été modifié.\nVoulez-vous sauvegarder ?", 
                    "Modifications non sauvegardées", MB_YESNOCANCEL | MB_ICONQUESTION);
                if (result == IDCANCEL) return 0;
                if (result == IDYES) {
                    SendMessageA(hwnd, WM_COMMAND, 1001, 0); // Déclencher la sauvegarde
                }
            }
            
            if (data) {
                if (data->fileData) HeapFree(GetProcessHeap(), 0, data->fileData);
                HeapFree(GetProcessHeap(), 0, data);
            }
            DestroyWindow(hwnd);
            return 0;
        }
        
        case WM_DESTROY: {
            return 0;
        }
    }
    
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// Affiche le viewer ASCII
void ShowASCIIViewer(const char* filename) {
    ViewerData* data = (ViewerData*)HeapAlloc(GetProcessHeap(), 0, sizeof(ViewerData));
    if (!data) return;
    
    lstrcpyA(data->filePath, filename);
    data->isEditor = FALSE;
    data->isHex = FALSE;
    data->isModified = FALSE;
    data->hEdit = NULL;
    data->hSaveButton = NULL;
    data->hCloseButton = NULL;
    
    if (!LoadFileData(filename, &data->fileData, &data->fileSize)) {
        HeapFree(GetProcessHeap(), 0, data);
        ShowCenteredMessageBox("Impossible de charger le fichier", "Erreur", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Enregistrer la classe de fenêtre
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = ViewerWindowProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "ViewerWnd";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    
    // Obtenir la taille de la fenêtre principale
    RECT mainRect;
    GetWindowRect(g_hMainWindow, &mainRect);
    int width = mainRect.right - mainRect.left;
    int height = mainRect.bottom - mainRect.top;
    
    char title[300];
    sprintf(title, "ASCII Viewer - %s", filename);
    
    HWND hViewerWnd = CreateWindowA("ViewerWnd", title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        mainRect.left, mainRect.top, width, height, NULL, NULL, GetModuleHandleA(NULL), data);
    
    ShowWindow(hViewerWnd, SW_SHOW);
}

// Affiche le viewer HEX
void ShowHEXViewer(const char* filename) {
    ViewerData* data = (ViewerData*)HeapAlloc(GetProcessHeap(), 0, sizeof(ViewerData));
    if (!data) return;
    
    lstrcpyA(data->filePath, filename);
    data->isEditor = FALSE;
    data->isHex = TRUE;
    data->isModified = FALSE;
    data->hEdit = NULL;
    data->hSaveButton = NULL;
    data->hCloseButton = NULL;
    
    if (!LoadFileData(filename, &data->fileData, &data->fileSize)) {
        HeapFree(GetProcessHeap(), 0, data);
        ShowCenteredMessageBox("Impossible de charger le fichier", "Erreur", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Enregistrer la classe de fenêtre
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = ViewerWindowProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "ViewerWnd";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    
    // Obtenir la taille de la fenêtre principale
    RECT mainRect;
    GetWindowRect(g_hMainWindow, &mainRect);
    int width = mainRect.right - mainRect.left;
    int height = mainRect.bottom - mainRect.top;
    
    char title[300];
    sprintf(title, "HEX Viewer - %s", filename);
    
    HWND hViewerWnd = CreateWindowA("ViewerWnd", title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        mainRect.left, mainRect.top, width, height, NULL, NULL, GetModuleHandleA(NULL), data);
    
    ShowWindow(hViewerWnd, SW_SHOW);
}

// Affiche l'éditeur ASCII
void ShowASCIIEditor(const char* filename) {
    ViewerData* data = (ViewerData*)HeapAlloc(GetProcessHeap(), 0, sizeof(ViewerData));
    if (!data) return;
    
    lstrcpyA(data->filePath, filename);
    data->isEditor = TRUE;
    data->isHex = FALSE;
    data->isModified = FALSE;
    data->hEdit = NULL;
    data->hSaveButton = NULL;
    data->hCloseButton = NULL;
    
    if (!LoadFileData(filename, &data->fileData, &data->fileSize)) {
        HeapFree(GetProcessHeap(), 0, data);
        ShowCenteredMessageBox("Impossible de charger le fichier", "Erreur", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Enregistrer la classe de fenêtre
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = ViewerWindowProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "ViewerWnd";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    
    // Obtenir la taille de la fenêtre principale
    RECT mainRect;
    GetWindowRect(g_hMainWindow, &mainRect);
    int width = mainRect.right - mainRect.left;
    int height = mainRect.bottom - mainRect.top;
    
    char title[300];
    sprintf(title, "ASCII Editor - %s", filename);
    
    HWND hViewerWnd = CreateWindowA("ViewerWnd", title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        mainRect.left, mainRect.top, width, height, NULL, NULL, GetModuleHandleA(NULL), data);
    
    ShowWindow(hViewerWnd, SW_SHOW);
}

// Affiche l'éditeur HEX
void ShowHEXEditor(const char* filename) {
    ViewerData* data = (ViewerData*)HeapAlloc(GetProcessHeap(), 0, sizeof(ViewerData));
    if (!data) return;
    
    lstrcpyA(data->filePath, filename);
    data->isEditor = TRUE;
    data->isHex = TRUE;
    data->isModified = FALSE;
    data->hEdit = NULL;
    data->hSaveButton = NULL;
    data->hCloseButton = NULL;
    
    if (!LoadFileData(filename, &data->fileData, &data->fileSize)) {
        HeapFree(GetProcessHeap(), 0, data);
        ShowCenteredMessageBox("Impossible de charger le fichier", "Erreur", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Enregistrer la classe de fenêtre
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = ViewerWindowProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "ViewerWnd";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    
    // Obtenir la taille de la fenêtre principale
    RECT mainRect;
    GetWindowRect(g_hMainWindow, &mainRect);
    int width = mainRect.right - mainRect.left;
    int height = mainRect.bottom - mainRect.top;
    
    char title[300];
    sprintf(title, "HEX Editor - %s", filename);
    
    HWND hViewerWnd = CreateWindowA("ViewerWnd", title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        mainRect.left, mainRect.top, width, height, NULL, NULL, GetModuleHandleA(NULL), data);
    
    ShowWindow(hViewerWnd, SW_SHOW);
}