#include <windows.h>
#include <string.h>
#include "prototypes.h"

// =============================================================================
// RENAME.C - Fonctions de renommage de fichiers et dossiers
// =============================================================================

// Variables externes utilisées par les fonctions de renommage
extern HWND g_hMainWindow;
extern HFONT g_hFontUTF8;
extern char g_renameResult[MAX_PATH];
extern BOOL g_renameConfirmed;

// IDs des contrôles pour le dialogue de renommage
#define ID_RENAME_EDIT      4001
#define ID_RENAME_OK        4002
#define ID_RENAME_CANCEL    4003

// =============================================================================
// Procédure de fenêtre pour le dialogue de renommage
// =============================================================================
LRESULT CALLBACK RenameDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Titre
            CreateWindowA("STATIC", "Nouveau nom :",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 10, 280, 20, hwnd, NULL, NULL, NULL);
            
            // Zone de texte pour le nouveau nom
            HWND hEdit = CreateWindowA("EDIT", g_renameResult,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                10, 35, 280, 25, hwnd, (HMENU)ID_RENAME_EDIT, NULL, NULL);
            
            // Boutons
            CreateWindowA("BUTTON", "OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                70, 75, 60, 25, hwnd, (HMENU)ID_RENAME_OK, NULL, NULL);
                
            CreateWindowA("BUTTON", "Annuler",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                140, 75, 60, 25, hwnd, (HMENU)ID_RENAME_CANCEL, NULL, NULL);
            
            // Appliquer police UTF-8
            if (g_hFontUTF8) {
                SendMessageA(hEdit, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                for (int id = ID_RENAME_OK; id <= ID_RENAME_CANCEL; id++) {
                    HWND hCtrl = GetDlgItem(hwnd, id);
                    if (hCtrl) SendMessageA(hCtrl, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                }
            }
            
            // Sélectionner tout le texte
            SetFocus(hEdit);
            SendMessageA(hEdit, EM_SETSEL, 0, -1);
            break;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            
            if (wmId == ID_RENAME_OK || wmId == IDOK) {
                // Récupérer le nouveau nom
                GetWindowTextA(GetDlgItem(hwnd, ID_RENAME_EDIT), g_renameResult, MAX_PATH);
                g_renameConfirmed = TRUE;
                DestroyWindow(hwnd);
            } else if (wmId == ID_RENAME_CANCEL || wmId == IDCANCEL) {
                g_renameConfirmed = FALSE;
                DestroyWindow(hwnd);
            } else if (wmId == ID_RENAME_EDIT && wmEvent == EN_CHANGE) {
                // Zone de texte modifiée - pas d'action particulière
            }
            break;
        }
        
        case WM_CLOSE:
            g_renameConfirmed = FALSE;
            DestroyWindow(hwnd);
            break;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// =============================================================================
// Affiche la boîte de dialogue de renommage
// =============================================================================
BOOL ShowRenameDialog(const char* currentName, char* newName) {
    // Préparer le nom actuel
    lstrcpynA(g_renameResult, currentName, MAX_PATH);
    g_renameConfirmed = FALSE;
    
    // Créer et afficher la boîte de dialogue modale
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = RenameDialogProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "RenameDlg";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    
    // Enregistrer la classe seulement si elle n'existe pas déjà
    if (!GetClassInfoA(GetModuleHandleA(NULL), "RenameDlg", &wc)) {
        RegisterClassA(&wc);
    }
    
    HWND hDlg = CreateCenteredWindow("RenameDlg", "Renommer",
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
    
    if (g_renameConfirmed) {
        lstrcpyA(newName, g_renameResult);
        return TRUE;
    }
    return FALSE;
}