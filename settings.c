#include <windows.h>
#include <commctrl.h>
#include "prototypes.h"

// Variables externes
extern AppSettings g_settings;
extern HWND g_hMainWindow;
extern HWND g_hListLeft, g_hListRight;
extern char g_szLeftPath[MAX_PATH], g_szRightPath[MAX_PATH];
extern HFONT g_hFontUTF8;

// Charge les préférences depuis le fichier
void LoadSettings() {
    HANDLE hFile = CreateFileA("FileMaster.prefs", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytesRead;
        ReadFile(hFile, &g_settings, sizeof(AppSettings), &bytesRead, NULL);
        CloseHandle(hFile);
    }
}

// Sauvegarde les préférences dans le fichier
void SaveSettings() {
    HANDLE hFile = CreateFileA("FileMaster.prefs", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(hFile, &g_settings, sizeof(AppSettings), &bytesWritten, NULL);
        CloseHandle(hFile);
    }
}

// Fenêtre de dialogue pour les paramètres
LRESULT CALLBACK SettingsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Section Général
            CreateWindowA("STATIC", "=== GENERAL ===",
                WS_CHILD | WS_VISIBLE,
                10, 10, 200, 20, hwnd, NULL, NULL, NULL);
                
            // Checkbox pour fichiers cachés
            HWND hCheckbox = CreateWindowA("BUTTON", "Afficher les fichiers cach\xE9s",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                10, 35, 200, 25, hwnd, (HMENU)2001, NULL, NULL);
            
            // Combo pour mode de tri
            CreateWindowA("STATIC", "Mode de tri:",
                WS_CHILD | WS_VISIBLE,
                10, 70, 100, 20, hwnd, NULL, NULL, NULL);
                
            HWND hCombo = CreateWindowA("COMBOBOX", NULL,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                10, 95, 150, 100, hwnd, (HMENU)2002, NULL, NULL);
                
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"Par nom");
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"Par taille");
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"Par date");
            SendMessageA(hCombo, CB_SETCURSEL, g_settings.sortMode, 0);
            
            // Section Polices
            CreateWindowA("STATIC", "=== POLICES ===",
                WS_CHILD | WS_VISIBLE,
                10, 130, 200, 20, hwnd, NULL, NULL, NULL);
                
            // Police panneaux
            CreateWindowA("STATIC", "Panneaux fichiers:",
                WS_CHILD | WS_VISIBLE,
                10, 155, 120, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("EDIT", g_settings.fontPanels,
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                140, 155, 100, 20, hwnd, (HMENU)2003, NULL, NULL);
            CreateWindowA("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                250, 155, 30, 20, hwnd, (HMENU)2004, NULL, NULL);
                
            // Police boutons
            CreateWindowA("STATIC", "Boutons:",
                WS_CHILD | WS_VISIBLE,
                10, 180, 120, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("EDIT", g_settings.fontButtons,
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                140, 180, 100, 20, hwnd, (HMENU)2005, NULL, NULL);
            CreateWindowA("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                250, 180, 30, 20, hwnd, (HMENU)2006, NULL, NULL);
                
            // Police interface
            CreateWindowA("STATIC", "Interface:",
                WS_CHILD | WS_VISIBLE,
                10, 205, 120, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("EDIT", g_settings.fontSettings,
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                140, 205, 100, 20, hwnd, (HMENU)2007, NULL, NULL);
            CreateWindowA("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                250, 205, 30, 20, hwnd, (HMENU)2008, NULL, NULL);
                
            // Police barres statut
            CreateWindowA("STATIC", "Barres statut:",
                WS_CHILD | WS_VISIBLE,
                10, 230, 120, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("EDIT", g_settings.fontStatusBar,
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                140, 230, 100, 20, hwnd, (HMENU)2009, NULL, NULL);
            CreateWindowA("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                250, 230, 30, 20, hwnd, (HMENU)2010, NULL, NULL);
            
            // Initialiser les tailles de police
            char szSize[10];
            wsprintfA(szSize, "%d", g_settings.fontSizePanels);
            SetWindowTextA(GetDlgItem(hwnd, 2004), szSize);
            wsprintfA(szSize, "%d", g_settings.fontSizeButtons);
            SetWindowTextA(GetDlgItem(hwnd, 2006), szSize);
            wsprintfA(szSize, "%d", g_settings.fontSizeSettings);
            SetWindowTextA(GetDlgItem(hwnd, 2008), szSize);
            wsprintfA(szSize, "%d", g_settings.fontSizeStatusBar);
            SetWindowTextA(GetDlgItem(hwnd, 2010), szSize);
            
            // Boutons OK et Annuler centrés en bas
            HWND hOK = CreateWindowA("BUTTON", "OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                150, 320, 60, 30, hwnd, (HMENU)IDOK, NULL, NULL);
                
            HWND hCancel = CreateWindowA("BUTTON", "Annuler",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                220, 320, 60, 30, hwnd, (HMENU)IDCANCEL, NULL, NULL);
            
            // Appliquer la police UTF-8 à tous les contrôles
            if (g_hFontUTF8) {
                SendMessageA(hCheckbox, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                SendMessageA(hCombo, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                SendMessageA(hOK, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                SendMessageA(hCancel, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                
                // Appliquer aux labels et champs de texte
                for (int i = 2003; i <= 2010; i++) {
                    HWND hCtrl = GetDlgItem(hwnd, i);
                    if (hCtrl) SendMessageA(hCtrl, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                }
            }
            
            // Initialiser checkbox
            SendMessageA(hCheckbox, BM_SETCHECK, g_settings.showHidden ? BST_CHECKED : BST_UNCHECKED, 0);
            break;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDOK) {
                // Récupérer les valeurs des contrôles généraux
                g_settings.showHidden = SendMessageA(GetDlgItem(hwnd, 2001), BM_GETCHECK, 0, 0);
                g_settings.sortMode = SendMessageA(GetDlgItem(hwnd, 2002), CB_GETCURSEL, 0, 0);
                
                // Récupérer les polices
                GetWindowTextA(GetDlgItem(hwnd, 2003), g_settings.fontPanels, sizeof(g_settings.fontPanels));
                GetWindowTextA(GetDlgItem(hwnd, 2005), g_settings.fontButtons, sizeof(g_settings.fontButtons));
                GetWindowTextA(GetDlgItem(hwnd, 2007), g_settings.fontSettings, sizeof(g_settings.fontSettings));
                GetWindowTextA(GetDlgItem(hwnd, 2009), g_settings.fontStatusBar, sizeof(g_settings.fontStatusBar));
                
                // Récupérer les tailles de police
                char szSize[10];
                GetWindowTextA(GetDlgItem(hwnd, 2004), szSize, sizeof(szSize));
                g_settings.fontSizePanels = simple_atoi(szSize);
                GetWindowTextA(GetDlgItem(hwnd, 2006), szSize, sizeof(szSize));
                g_settings.fontSizeButtons = simple_atoi(szSize);
                GetWindowTextA(GetDlgItem(hwnd, 2008), szSize, sizeof(szSize));
                g_settings.fontSizeSettings = simple_atoi(szSize);
                GetWindowTextA(GetDlgItem(hwnd, 2010), szSize, sizeof(szSize));
                g_settings.fontSizeStatusBar = simple_atoi(szSize);
                
                // Valider les tailles (minimum 8, maximum 72)
                if (g_settings.fontSizePanels < 8) g_settings.fontSizePanels = 8;
                if (g_settings.fontSizePanels > 72) g_settings.fontSizePanels = 72;
                if (g_settings.fontSizeButtons < 8) g_settings.fontSizeButtons = 8;
                if (g_settings.fontSizeButtons > 72) g_settings.fontSizeButtons = 72;
                if (g_settings.fontSizeSettings < 8) g_settings.fontSizeSettings = 8;
                if (g_settings.fontSizeSettings > 72) g_settings.fontSizeSettings = 72;
                if (g_settings.fontSizeStatusBar < 8) g_settings.fontSizeStatusBar = 8;
                if (g_settings.fontSizeStatusBar > 72) g_settings.fontSizeStatusBar = 72;
                
                SaveSettings();
                
                // Message pour redémarrer l'application
                ShowCenteredMessageBox("Polices sauvegardées.\nRedémarrez l'application pour voir les changements.", 
                    "Paramètres", MB_OK | MB_ICONINFORMATION);
                
                // Appliquer les paramètres généraux : rafraîchir les panneaux
                RefreshPanel(g_hListLeft, g_szLeftPath);
                RefreshPanel(g_hListRight, g_szRightPath);
                
                // Réactiver la fenêtre principale et fermer
                EnableWindow(g_hMainWindow, TRUE);
                SetForegroundWindow(g_hMainWindow);
                DestroyWindow(hwnd);
            } else if (wmId == IDCANCEL) {
                // Annuler : réactiver la fenêtre principale et fermer sans sauvegarder
                EnableWindow(g_hMainWindow, TRUE);
                SetForegroundWindow(g_hMainWindow);
                DestroyWindow(hwnd);
            }
            break;
        }
        
        case WM_CLOSE:
            // Réactiver la fenêtre principale
            EnableWindow(g_hMainWindow, TRUE);
            SetForegroundWindow(g_hMainWindow);
            DestroyWindow(hwnd);
            break;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// Affiche la fenêtre de paramètres (modale)
void ShowSettings() {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = SettingsProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "SettingsWnd";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    
    RegisterClassA(&wc);
    
    // Désactiver la fenêtre principale pour la modalité
    EnableWindow(g_hMainWindow, FALSE);
    
    HWND hSettingsWnd = CreateCenteredWindow("SettingsWnd", "FileMaster - Param\xE8tres",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 450, 400, g_hMainWindow, GetModuleHandleA(NULL));
    
    // Appliquer la police UTF-8 à la fenêtre de settings
    if (g_hFontUTF8) {
        SendMessageA(hSettingsWnd, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
    }
        
    ShowWindow(hSettingsWnd, SW_SHOW);
    SetForegroundWindow(hSettingsWnd);
}