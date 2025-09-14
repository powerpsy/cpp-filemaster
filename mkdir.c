#include <windows.h>
#include <string.h>
#include "prototypes.h"

// =============================================================================
// MKDIR.C - Fonctions de création de dossiers
// =============================================================================

// Variables externes utilisées par les fonctions de création de dossier
extern HWND g_hMainWindow;
extern HFONT g_hFontUTF8;
extern char g_makedirResult[MAX_PATH];
extern BOOL g_makedirConfirmed;

// IDs des contrôles pour le dialogue de création de dossier
#define ID_MAKEDIR_EDIT     5001
#define ID_MAKEDIR_OK       5002
#define ID_MAKEDIR_CANCEL   5003

// =============================================================================
// Procédure de fenêtre pour le dialogue de création de répertoire
// =============================================================================
LRESULT CALLBACK MakedirDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Titre
            CreateWindowA("STATIC", "Nom du nouveau répertoire :",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 10, 280, 20, hwnd, NULL, NULL, NULL);
            
            // Zone de texte pour le nom du répertoire
            HWND hEdit = CreateWindowA("EDIT", g_makedirResult,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                10, 35, 280, 25, hwnd, (HMENU)ID_MAKEDIR_EDIT, NULL, NULL);
            
            // Boutons
            CreateWindowA("BUTTON", "OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                70, 75, 60, 25, hwnd, (HMENU)ID_MAKEDIR_OK, NULL, NULL);
                
            CreateWindowA("BUTTON", "Annuler",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                140, 75, 60, 25, hwnd, (HMENU)ID_MAKEDIR_CANCEL, NULL, NULL);
            
            // Appliquer police UTF-8
            if (g_hFontUTF8) {
                SendMessageA(hEdit, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                for (int id = ID_MAKEDIR_OK; id <= ID_MAKEDIR_CANCEL; id++) {
                    HWND hCtrl = GetDlgItem(hwnd, id);
                    if (hCtrl) SendMessageA(hCtrl, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                }
            }
            
            // Mettre le focus sur la zone de texte
            SetFocus(hEdit);
            break;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            
            if (wmId == ID_MAKEDIR_OK || wmId == IDOK) {
                // Récupérer le nom du répertoire
                GetWindowTextA(GetDlgItem(hwnd, ID_MAKEDIR_EDIT), g_makedirResult, MAX_PATH);
                g_makedirConfirmed = TRUE;
                DestroyWindow(hwnd);
            } else if (wmId == ID_MAKEDIR_CANCEL || wmId == IDCANCEL) {
                g_makedirConfirmed = FALSE;
                DestroyWindow(hwnd);
            } else if (wmId == ID_MAKEDIR_EDIT && wmEvent == EN_CHANGE) {
                // Zone de texte modifiée - pas d'action particulière
            }
            break;
        }
        
        case WM_CLOSE:
            g_makedirConfirmed = FALSE;
            DestroyWindow(hwnd);
            break;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// =============================================================================
// Affiche la boîte de dialogue de création de répertoire
// =============================================================================
BOOL ShowMakedirDialog(char* dirName) {
    // Initialiser le nom par défaut
    lstrcpyA(g_makedirResult, "");
    g_makedirConfirmed = FALSE;
    
    // Créer et afficher la boîte de dialogue modale
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = MakedirDialogProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "MakedirDlg";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    
    // Enregistrer la classe seulement si elle n'existe pas déjà
    if (!GetClassInfoA(GetModuleHandleA(NULL), "MakedirDlg", &wc)) {
        RegisterClassA(&wc);
    }
    
    HWND hDlg = CreateCenteredWindow("MakedirDlg", "Créer un répertoire",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 320, 130, g_hMainWindow, GetModuleHandleA(NULL));
    
    // Rendre modal
    EnableWindow(g_hMainWindow, FALSE);
    ShowWindow(hDlg, SW_SHOW);
    SetForegroundWindow(hDlg);
    
    // Boucle de messages pour la modalité
    MSG msg;
    while (IsWindow(hDlg)) {
        if (GetMessageA(&msg, NULL, 0, 0)) {
            if (msg.hwnd == hDlg || IsChild(hDlg, msg.hwnd)) {
                // Utiliser IsDialogMessage pour gérer les touches comme TAB, RETURN, ESC
                if (!IsDialogMessageA(hDlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                }
            } else {
                // Messages pour d'autres fenêtres - les ignorer pour maintenir la modalité
                if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) {
                    continue;
                }
                if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST) {
                    continue;
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        } else {
            break;
        }
    }
    
    // Réactiver la fenêtre principale
    EnableWindow(g_hMainWindow, TRUE);
    SetForegroundWindow(g_hMainWindow);
    
    if (g_makedirConfirmed) {
        lstrcpyA(dirName, g_makedirResult);
        return TRUE;
    }
    return FALSE;
}