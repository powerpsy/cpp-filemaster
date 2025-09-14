#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <shellapi.h>
#include "prototypes.h"

// IDs pour les contrôles
#define ID_LISTBOX_LEFT     1001
#define ID_LISTBOX_RIGHT    1002
#define ID_BTN_CEPC         1003
#define ID_BTN_PARENT       1004
#define ID_BTN_INVERT       1005
#define ID_BTN_CLEAR        1006
#define ID_BTN_COPY         1007
#define ID_BTN_MOVE         1008
#define ID_BTN_DELETE       1009
#define ID_BTN_RENAME       1010
#define ID_BTN_MAKEDIR      1011
#define ID_BTN_SHOW_ASC     1012
#define ID_BTN_SHOW_HEX     1013
#define ID_BTN_EDIT         1014
#define ID_BTN_EDIT_HEX     1015
#define ID_BTN_EXECUTE      1016
#define ID_BTN_SETTINGS     1017
#define ID_BTN_QUIT         1018
#define ID_BTN_ARROW_RIGHT  1019
#define ID_BTN_ARROW_LEFT   1020

// IDs pour la boîte de dialogue de copie
#define ID_COPY_REPLACE     3001
#define ID_COPY_SKIP        3002  
#define ID_COPY_CANCEL      3003
#define ID_COPY_APPLY_ALL   3004

// IDs pour la boîte de dialogue de renommage
#define ID_RENAME_EDIT      4001
#define ID_RENAME_OK        4002
#define ID_RENAME_CANCEL    4003

// IDs pour la boîte de dialogue de création de répertoire
#define ID_MAKEDIR_EDIT     4004
#define ID_MAKEDIR_OK       4005
#define ID_MAKEDIR_CANCEL   4006

// Variables globales pour la gestion de conflits
int g_copyChoice = 0; // 0=demander, 1=remplacer, 2=passer, 3=annuler
BOOL g_copyApplyAll = FALSE;

// Variables pour les informations de conflit
char g_sourceInfo[512];
char g_destInfo[512];

// Variables pour le renommage
char g_renameResult[MAX_PATH];
BOOL g_renameConfirmed = FALSE;

// Variables pour la création de répertoire
char g_makedirResult[MAX_PATH];
BOOL g_makedirConfirmed = FALSE;

// Variables pour le sous-classement des listes
WNDPROC g_oldListProcLeft = NULL;
WNDPROC g_oldListProcRight = NULL;
int g_lastClickedItem = -1;

// Variables globales
HWND g_hListLeft, g_hListRight;
HWND g_hMainWindow, g_hStatusLeft, g_hStatusRight;
HWND g_hArrowRight, g_hArrowLeft; // Boutons flèches pour synchroniser les répertoires
char g_szLeftPath[MAX_PATH] = "C:\\";
char g_szRightPath[MAX_PATH] = "C:\\";
int g_nActivePanel = 0; // 0=gauche, 1=droite

// Couleurs pour les panneaux
HBRUSH g_hBrushActive = NULL;   // RGB(102,136,187)
HBRUSH g_hBrushInactive = NULL; // RGB(170,170,170)

// Police UTF-8 globale
HFONT g_hFontUTF8 = NULL;
HFONT g_hFontMono = NULL;

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

// Fonction pour centrer toutes les MessageBox
// Hook pour centrer MessageBox
HHOOK g_hHook = NULL;
HWND g_hMsgBoxWnd = NULL;

// Variables globales  
int g_currentPanel = 0; // 0 = gauche, 1 = droite
char g_szLeftPath[MAX_PATH] = "C:\\";
char g_szRightPath[MAX_PATH] = "C:\\";
char g_szSelectedFiles[10][MAX_PATH];
int g_nSelectedCount = 0;
HWND g_hMainWindow = NULL;
HWND g_hListLeft = NULL, g_hListRight = NULL;
HWND g_hStatusLeft = NULL, g_hStatusRight = NULL;
HHOOK g_hHook = NULL;
HFONT g_hFontUTF8 = NULL;
HFONT g_hFontMono = NULL;

// Structure pour les préférences (définie aussi dans settings.c)
typedef struct {
    int showHidden;
    int sortMode; // 0=nom, 1=taille, 2=date
    char defaultPath[MAX_PATH];
    // Configuration des polices
    char fontPanels[64];     // Police pour panneaux 1&2 (listes fichiers)
    char fontButtons[64];    // Police pour panneau 3 (boutons)
    char fontTitle[64];      // Police pour titre application
    char fontSettings[64];   // Police pour fenêtre settings
    char fontStatusBar[64];  // Police pour barres de statut
    int fontSizePanels;
    int fontSizeButtons;
    int fontSizeTitle;
    int fontSizeSettings;
    int fontSizeStatusBar;
} AppSettings;

AppSettings g_settings = {0, 0, "C:\\", 
    "Consolas", "Segoe UI", "Segoe UI", "Segoe UI", "Segoe UI",
    12, 9, 12, 12, 12};

// Fonctions de gestion des fichiers pour les viewers/éditeurs
// Refresh un panneau avec le contenu d'un répertoire
void RefreshPanel(HWND hList, const char* szPath) {
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    
    // Mettre à jour la barre de statut correspondante
    if (hList == g_hListLeft) {
        SetWindowTextA(g_hStatusLeft, szPath);
    } else {
        SetWindowTextA(g_hStatusRight, szPath);
    }
    
    char szSearch[MAX_PATH];
    wsprintfA(szSearch, "%s*", szPath);
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(szSearch, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (lstrcmpA(findData.cFileName, ".") != 0) {
                // Vérifier si on doit afficher les fichiers cachés
                BOOL isHidden = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
                if (isHidden && !g_settings.showHidden) {
                    continue; // Ignorer ce fichier caché
                }
                
                char szEntry[400];
                char szSize[50];
                char szDisplayName[40]; // 35 + quelques caractères de sécurité
                
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // Tronquer le nom si nécessaire
                    if (lstrlenA(findData.cFileName) > 35) {
                        lstrcpynA(szDisplayName, findData.cFileName, 33); // copie 32 chars + null
                        szDisplayName[32] = '.';
                        szDisplayName[33] = '.';
                        szDisplayName[34] = '.';
                        szDisplayName[35] = '\0';
                    } else {
                        lstrcpyA(szDisplayName, findData.cFileName);
                    }
                    // Format: "nom (35 max) + espace + DIR (2 char)"
                    wsprintfA(szEntry, "%-35s %2s", szDisplayName, "DIR");
                } else {
                    // Tronquer le nom si nécessaire
                    if (lstrlenA(findData.cFileName) > 35) {
                        lstrcpynA(szDisplayName, findData.cFileName, 33); // copie 32 chars + null
                        szDisplayName[32] = '.';
                        szDisplayName[33] = '.';
                        szDisplayName[34] = '.';
                        szDisplayName[35] = '\0';
                    } else {
                        lstrcpyA(szDisplayName, findData.cFileName);
                    }
                    // Format: "nom (35 max) + espace + taille (5 char) + espace + unité (2 char)"
                    FormatFileSize(findData.nFileSizeLow, findData.nFileSizeHigh, szSize);
                    wsprintfA(szEntry, "%-35s %s", szDisplayName, szSize);
                }
                SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)szEntry);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
}

// Récupère le nom réel d'un fichier à partir de son index dans la liste
BOOL GetRealFileName(const char* szPath, int index, char* szRealName) {
    char szSearch[MAX_PATH];
    wsprintfA(szSearch, "%s*", szPath);
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(szSearch, &findData);
    int currentIndex = 0;
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (lstrcmpA(findData.cFileName, ".") != 0) {
                // Vérifier si on doit afficher les fichiers cachés
                BOOL isHidden = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
                if (isHidden && !g_settings.showHidden) {
                    continue; // Ignorer ce fichier caché
                }
                
                if (currentIndex == index) {
                    lstrcpyA(szRealName, findData.cFileName);
                    FindClose(hFind);
                    return TRUE;
                }
                currentIndex++;
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    return FALSE;
}

// Procédure de fenêtre pour la création de répertoire

// Procédure de sous-classement pour la sélection Windows-style
LRESULT CALLBACK ListBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_LBUTTONDOWN) {
        POINT pt = {LOWORD(lParam), HIWORD(lParam)};
        int item = SendMessageA(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
        
        if (HIWORD(item) == 0) { // Clic valide sur un item
            item = LOWORD(item);
            BOOL ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            BOOL shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            
            if (ctrlPressed) {
                // CTRL+clic : toggle de la sélection de l'item
                int isSelected = SendMessageA(hwnd, LB_GETSEL, item, 0);
                SendMessageA(hwnd, LB_SETSEL, !isSelected, item);
                g_lastClickedItem = item;
            } else if (shiftPressed && g_lastClickedItem != -1) {
                // SHIFT+clic : sélection d'une plage
                int start = (g_lastClickedItem < item) ? g_lastClickedItem : item;
                int end = (g_lastClickedItem < item) ? item : g_lastClickedItem;
                
                // Désélectionner tout d'abord
                SendMessageA(hwnd, LB_SETSEL, FALSE, -1);
                
                // Sélectionner la plage
                for (int i = start; i <= end; i++) {
                    SendMessageA(hwnd, LB_SETSEL, TRUE, i);
                }
            } else {
                // Clic simple : désélectionner tout et sélectionner uniquement cet item
                SendMessageA(hwnd, LB_SETSEL, FALSE, -1);
                SendMessageA(hwnd, LB_SETSEL, TRUE, item);
                g_lastClickedItem = item;
            }
            
            // Changer le panneau actif
            g_nActivePanel = (hwnd == g_hListLeft) ? 0 : 1;
            
            // Forcer le redraw des deux panneaux pour mettre à jour les couleurs
            InvalidateRect(g_hListLeft, NULL, TRUE);
            InvalidateRect(g_hListRight, NULL, TRUE);
            
            return 0; // Empêcher le traitement par défaut
        }
    }
    
    // Appeler la procédure d'origine pour les autres messages
    if (hwnd == g_hListLeft && g_oldListProcLeft) {
        return CallWindowProcA(g_oldListProcLeft, hwnd, uMsg, wParam, lParam);
    } else if (hwnd == g_hListRight && g_oldListProcRight) {
        return CallWindowProcA(g_oldListProcRight, hwnd, uMsg, wParam, lParam);
    }
    
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// Liste les lecteurs disponibles
void ListDrives(HWND hList) {
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            char szDrive[50];
            char szDriveName[10];
            wsprintfA(szDriveName, "%c:", 'A' + i);
            wsprintfA(szDrive, "%-29s %8s", szDriveName, "DRIVE");
            SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)szDrive);
        }
    }
}

// Remonte au répertoire parent
void GoToParent(int panel) {
    char* szPath = (panel == 0) ? g_szLeftPath : g_szRightPath;
    HWND hList = (panel == 0) ? g_hListLeft : g_hListRight;
    
    int len = lstrlenA(szPath);
    if (len > 3) { // Pas à la racine
        for (int i = len - 2; i >= 0; i--) {
            if (szPath[i] == '\\') {
                szPath[i + 1] = '\0';
                break;
            }
        }
        RefreshPanel(hList, szPath);
    }
}

// Exécute une commande
            
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
        
        case WM_KEYDOWN: {
            if (wParam == VK_RETURN) {
                // RETURN = OK (même traitement que le bouton OK)
                SendMessageA(hwnd, WM_COMMAND, IDOK, 0);
                return 0;
            } else if (wParam == VK_ESCAPE) {
                // ESC = Annuler (même traitement que le bouton Annuler)
                SendMessageA(hwnd, WM_COMMAND, IDCANCEL, 0);
                return 0;
            }
            break;
        }
        
        case WM_CLOSE:
            // Réactiver la fenêtre principale et fermer
            EnableWindow(g_hMainWindow, TRUE);
            SetForegroundWindow(g_hMainWindow);
            DestroyWindow(hwnd);
            break;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// Exécute une commande
void ExecuteCommand(int cmdId) {
    HWND hActiveList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
    char* szActivePath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
    
    switch (cmdId) {
        case ID_BTN_CEPC:
            // Lister les disques seulement sur le panneau actif
            if (g_nActivePanel == 0) {
                ListDrives(g_hListLeft);
                lstrcpyA(g_szLeftPath, "");
            } else {
                ListDrives(g_hListRight);
                lstrcpyA(g_szRightPath, "");
            }
            break;
            
        case ID_BTN_PARENT:
            GoToParent(g_nActivePanel);
            break;
            
        case ID_BTN_INVERT: {
            // Inverser la sélection dans le panneau actif
            HWND hList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            int count = SendMessageA(hList, LB_GETCOUNT, 0, 0);
            
            for (int i = 0; i < count; i++) {
                int isSelected = SendMessageA(hList, LB_GETSEL, i, 0);
                SendMessageA(hList, LB_SETSEL, !isSelected, i);
            }
            break;
        }
            
        case ID_BTN_CLEAR: {
            // Désélectionner tout dans le panneau actif
            HWND hList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            int count = SendMessageA(hList, LB_GETCOUNT, 0, 0);
            
            for (int i = 0; i < count; i++) {
                SendMessageA(hList, LB_SETSEL, FALSE, i);
            }
            break;
        }
            
        case ID_BTN_COPY: {
            // Copier les fichiers sélectionnés vers l'autre panneau
            HWND hSourceList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szSourcePath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            char* szDestPath = (g_nActivePanel == 0) ? g_szRightPath : g_szLeftPath;
            
            // Vérifier que le panneau de destination n'affiche pas les lecteurs
            if (strlen(szDestPath) == 0) {
                HWND hDestList = (g_nActivePanel == 0) ? g_hListRight : g_hListLeft;
                FlashError(hDestList);
                break;
            }
            
            int count = SendMessageA(hSourceList, LB_GETCOUNT, 0, 0);
            int copiedFiles = 0;
            int skippedFiles = 0;
            BOOL operationCancelled = FALSE;
            
            // Réinitialiser les choix de conflit
            g_copyChoice = 0;
            g_copyApplyAll = FALSE;
            
            for (int i = 0; i < count && !operationCancelled; i++) {
                int isSelected = SendMessageA(hSourceList, LB_GETSEL, i, 0);
                if (isSelected) {
                    // Récupérer le nom réel du fichier (pas l'affichage tronqué)
                    char szRealFileName[MAX_PATH];
                    if (!GetRealFileName(szSourcePath, i, szRealFileName)) {
                        continue; // Erreur lors de la récupération du nom
                    }
                    
                    // Ignorer le dossier parent ".."
                    if (lstrcmpA(szRealFileName, "..") == 0) continue;
                    
                    // Construire les chemins complets
                    char szSourceFile[MAX_PATH];
                    char szDestFile[MAX_PATH];
                    wsprintfA(szSourceFile, "%s%s", szSourcePath, szRealFileName);
                    wsprintfA(szDestFile, "%s%s", szDestPath, szRealFileName);
                    
                    // Vérifier si c'est un dossier
                    DWORD attrs = GetFileAttributesA(szSourceFile);
                    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        // Ignorer les dossiers pour cette version simplifiée
                        continue;
                    }
                    
                    // Vérifier si le fichier de destination existe
                    BOOL fileExists = (GetFileAttributesA(szDestFile) != INVALID_FILE_ATTRIBUTES);
                    BOOL shouldCopy = TRUE;
                    
                    if (fileExists) {
                        int choice = ShowConflictDialog(szSourceFile, szDestFile);
                        switch (choice) {
                            case 1: // Remplacer
                                shouldCopy = TRUE;
                                break;
                            case 2: // Passer
                                shouldCopy = FALSE;
                                skippedFiles++;
                                break;
                            case 3: // Annuler tout
                                operationCancelled = TRUE;
                                shouldCopy = FALSE;
                                break;
                        }
                    }
                    
                    // Copier le fichier si nécessaire
                    if (shouldCopy && !operationCancelled) {
                        if (CopyFileA(szSourceFile, szDestFile, FALSE)) {
                            copiedFiles++;
                        }
                    }
                }
            }
            
            // Rafraîchir le panneau de destination si l'opération n'a pas été annulée
            if (!operationCancelled) {
                HWND hDestList = (g_nActivePanel == 0) ? g_hListRight : g_hListLeft;
                RefreshPanel(hDestList, szDestPath);
            }
            break;
        }
        
        case ID_BTN_MOVE: {
            // Déplacer les fichiers sélectionnés vers l'autre panneau
            HWND hSourceList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szSourcePath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            char* szDestPath = (g_nActivePanel == 0) ? g_szRightPath : g_szLeftPath;
            
            // Vérifier que le panneau de destination n'affiche pas les lecteurs
            if (strlen(szDestPath) == 0) {
                HWND hDestList = (g_nActivePanel == 0) ? g_hListRight : g_hListLeft;
                FlashError(hDestList);
                break;
            }
            
            int count = SendMessageA(hSourceList, LB_GETCOUNT, 0, 0);
            int movedFiles = 0;
            int skippedFiles = 0;
            BOOL operationCancelled = FALSE;
            
            // Réinitialiser les choix de conflit
            g_copyChoice = 0;
            g_copyApplyAll = FALSE;
            
            for (int i = 0; i < count && !operationCancelled; i++) {
                int isSelected = SendMessageA(hSourceList, LB_GETSEL, i, 0);
                if (isSelected) {
                    // Récupérer le nom réel du fichier (pas l'affichage tronqué)
                    char szRealFileName[MAX_PATH];
                    if (!GetRealFileName(szSourcePath, i, szRealFileName)) {
                        continue; // Erreur lors de la récupération du nom
                    }
                    
                    // Ignorer le dossier parent ".."
                    if (lstrcmpA(szRealFileName, "..") == 0) continue;
                    
                    // Construire les chemins complets
                    char szSourceFile[MAX_PATH];
                    char szDestFile[MAX_PATH];
                    wsprintfA(szSourceFile, "%s%s", szSourcePath, szRealFileName);
                    wsprintfA(szDestFile, "%s%s", szDestPath, szRealFileName);
                    
                    // Vérifier si c'est un dossier
                    DWORD attrs = GetFileAttributesA(szSourceFile);
                    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        // Ignorer les dossiers pour cette version simplifiée
                        continue;
                    }
                    
                    // Vérifier si le fichier de destination existe
                    BOOL fileExists = (GetFileAttributesA(szDestFile) != INVALID_FILE_ATTRIBUTES);
                    BOOL shouldMove = TRUE;
                    
                    if (fileExists) {
                        int choice = ShowConflictDialog(szSourceFile, szDestFile);
                        switch (choice) {
                            case 1: // Remplacer
                                shouldMove = TRUE;
                                break;
                            case 2: // Passer
                                shouldMove = FALSE;
                                skippedFiles++;
                                break;
                            case 3: // Annuler tout
                                operationCancelled = TRUE;
                                shouldMove = FALSE;
                                break;
                        }
                    }
                    
                    // Déplacer le fichier si nécessaire
                    if (shouldMove && !operationCancelled) {
                        // Copier le fichier d'abord
                        if (CopyFileA(szSourceFile, szDestFile, FALSE)) {
                            // Si la copie a réussi, supprimer le fichier source
                            if (DeleteFileA(szSourceFile)) {
                                movedFiles++;
                            } else {
                                // Si la suppression échoue, supprimer le fichier de destination pour éviter les doublons
                                DeleteFileA(szDestFile);
                            }
                        }
                    }
                }
            }
            
            // Rafraîchir les panneaux si l'opération n'a pas été annulée
            if (!operationCancelled) {
                // Rafraîchir les deux panneaux car les fichiers ont été supprimés du source
                RefreshPanel(hSourceList, szSourcePath);
                HWND hDestList = (g_nActivePanel == 0) ? g_hListRight : g_hListLeft;
                RefreshPanel(hDestList, szDestPath);
            }
            break;
        }
        
        case ID_BTN_DELETE: {
            // Supprimer les fichiers sélectionnés
            HWND hSourceList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szSourcePath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            // Vérifier que le panneau actif n'affiche pas les lecteurs
            if (strlen(szSourcePath) == 0) {
                FlashError(hSourceList);
                break;
            }
            
            int count = SendMessageA(hSourceList, LB_GETCOUNT, 0, 0);
            int selectedCount = 0;
            
            // Compter les fichiers sélectionnés
            for (int i = 0; i < count; i++) {
                if (SendMessageA(hSourceList, LB_GETSEL, i, 0)) {
                    selectedCount++;
                }
            }
            
            if (selectedCount == 0) {
                ShowCenteredMessageBox("Aucun fichier sélectionné", "Suppression", MB_OK | MB_ICONWARNING);
                break;
            }
            
            // Demander confirmation
            char szMsg[200];
            if (selectedCount == 1) {
                wsprintfA(szMsg, "Supprimer le fichier sélectionné ?");
            } else {
                wsprintfA(szMsg, "Supprimer les %d fichiers sélectionnés ?", selectedCount);
            }
            
            if (ShowCenteredMessageBox(szMsg, "Confirmer suppression", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                int deletedFiles = 0;
                int errorCount = 0;
                
                for (int i = 0; i < count; i++) {
                    int isSelected = SendMessageA(hSourceList, LB_GETSEL, i, 0);
                    if (isSelected) {
                        // Récupérer le nom réel du fichier
                        char szRealFileName[MAX_PATH];
                        if (!GetRealFileName(szSourcePath, i, szRealFileName)) {
                            errorCount++;
                            continue;
                        }
                        
                        // Ignorer le dossier parent ".."
                        if (lstrcmpA(szRealFileName, "..") == 0) continue;
                        
                        // Construire le chemin complet
                        char szFullPath[MAX_PATH];
                        wsprintfA(szFullPath, "%s%s", szSourcePath, szRealFileName);
                        
                        // Vérifier si c'est un dossier
                        DWORD attrs = GetFileAttributesA(szFullPath);
                        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                            // Tenter de supprimer le dossier (seulement s'il est vide)
                            if (RemoveDirectoryA(szFullPath)) {
                                deletedFiles++;
                            } else {
                                errorCount++;
                            }
                        } else {
                            // Supprimer le fichier
                            if (DeleteFileA(szFullPath)) {
                                deletedFiles++;
                            } else {
                                errorCount++;
                            }
                        }
                    }
                }
                
                // Rafraîchir le panneau
                RefreshPanel(hSourceList, szSourcePath);
            }
            break;
        }
        
        case ID_BTN_RENAME: {
            // Renommer les fichiers sélectionnés un par un
            HWND hSourceList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szSourcePath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            // Vérifier que le panneau actif n'affiche pas les lecteurs
            if (strlen(szSourcePath) == 0) {
                FlashError(hSourceList);
                break;
            }
            
            int count = SendMessageA(hSourceList, LB_GETCOUNT, 0, 0);
            int selectedCount = 0;
            
            // Compter les fichiers sélectionnés
            for (int i = 0; i < count; i++) {
                if (SendMessageA(hSourceList, LB_GETSEL, i, 0)) {
                    selectedCount++;
                }
            }
            
            if (selectedCount == 0) {
                ShowCenteredMessageBox("Aucun fichier sélectionné", "Renommer", MB_OK | MB_ICONWARNING);
                break;
            }
            
            int renamedFiles = 0;
            int skippedFiles = 0;
            BOOL operationCancelled = FALSE;
            
            // Traiter chaque fichier sélectionné
            for (int i = 0; i < count && !operationCancelled; i++) {
                if (SendMessageA(hSourceList, LB_GETSEL, i, 0)) {
                    // Récupérer le nom réel du fichier
                    char szRealFileName[MAX_PATH];
                    if (!GetRealFileName(szSourcePath, i, szRealFileName)) {
                        skippedFiles++;
                        continue;
                    }
                    
                    // Ignorer le dossier parent ".."
                    if (lstrcmpA(szRealFileName, "..") == 0) {
                        skippedFiles++;
                        continue;
                    }
                    
                    // Demander le nouveau nom
                    char szNewName[MAX_PATH];
                    if (ShowRenameDialog(szRealFileName, szNewName)) {
                        // Vérifier que le nouveau nom n'est pas vide
                        if (lstrlenA(szNewName) == 0) {
                            skippedFiles++;
                            continue;
                        }
                        
                        // Vérifier que le nom a changé
                        if (lstrcmpA(szRealFileName, szNewName) == 0) {
                            skippedFiles++;
                            continue;
                        }
                        
                        // Construire les chemins complets
                        char szOldPath[MAX_PATH];
                        char szNewPath[MAX_PATH];
                        wsprintfA(szOldPath, "%s%s", szSourcePath, szRealFileName);
                        wsprintfA(szNewPath, "%s%s", szSourcePath, szNewName);
                        
                        // Vérifier que le fichier destination n'existe pas déjà
                        if (GetFileAttributesA(szNewPath) != INVALID_FILE_ATTRIBUTES) {
                            char szMsg[400];
                            wsprintfA(szMsg, "Un fichier avec le nom '%s' existe déjà", szNewName);
                            ShowCenteredMessageBox(szMsg, "Renommer", MB_OK | MB_ICONWARNING);
                            skippedFiles++;
                            continue;
                        }
                        
                        // Renommer le fichier
                        if (MoveFileA(szOldPath, szNewPath)) {
                            renamedFiles++;
                        } else {
                            char szMsg[400];
                            wsprintfA(szMsg, "Erreur lors du renommage de '%s'", szRealFileName);
                            ShowCenteredMessageBox(szMsg, "Renommer", MB_OK | MB_ICONERROR);
                            skippedFiles++;
                        }
                    } else {
                        // L'utilisateur a annulé pour ce fichier
                        skippedFiles++;
                    }
                    
                    // Rafraîchir le panneau après chaque renommage pour voir les changements
                    if (renamedFiles > 0) {
                        RefreshPanel(hSourceList, szSourcePath);
                    }
                }
            }
            
            // Rafraîchir le panneau une dernière fois si nécessaire
            if (renamedFiles > 0) {
                RefreshPanel(hSourceList, szSourcePath);
            }
            
            break;
        }
        
        case ID_BTN_MAKEDIR: {
            // Créer un nouveau répertoire dans le panneau actif
            char* szCurrentPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            HWND hActiveList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            
            // Vérifier que le panneau actif n'affiche pas les lecteurs
            if (strlen(szCurrentPath) == 0) {
                FlashError(hActiveList);
                break;
            }
            
            char szDirName[MAX_PATH];
            if (ShowMakedirDialog(szDirName)) {
                // Vérifier que le nom n'est pas vide
                if (strlen(szDirName) == 0) {
                    ShowCenteredMessageBox("Le nom du répertoire ne peut pas être vide", "Erreur", MB_OK | MB_ICONERROR);
                    break;
                }
                
                // Construire le chemin complet
                char szFullPath[MAX_PATH];
                wsprintfA(szFullPath, "%s%s", szCurrentPath, szDirName);
                
                // Vérifier si le répertoire existe déjà
                if (GetFileAttributesA(szFullPath) != INVALID_FILE_ATTRIBUTES) {
                    ShowCenteredMessageBox("Un fichier ou répertoire avec ce nom existe déjà", "Erreur", MB_OK | MB_ICONERROR);
                    break;
                }
                
                // Créer le répertoire
                if (CreateDirectoryA(szFullPath, NULL)) {
                    // Rafraîchir le panneau actif pour afficher le nouveau répertoire
                    RefreshPanel(hActiveList, szCurrentPath);
                } else {
                    ShowCenteredMessageBox("Impossible de créer le répertoire", "Erreur", MB_OK | MB_ICONERROR);
                }
            }
            break;
        }
        
        case ID_BTN_SHOW_ASC: {
            HWND hActiveList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szCurrentPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            // Vérifier qu'on n'est pas dans la liste des lecteurs
            if (strlen(szCurrentPath) == 0) {
                FlashError(hActiveList);
                break;
            }
            
            int sel = SendMessageA(hActiveList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char szItem[400];
                SendMessageA(hActiveList, LB_GETTEXT, sel, (LPARAM)szItem);
                
                // Vérifier si c'est un fichier (pas un répertoire)
                if (!strstr(szItem, "DIR")) {
                    char szRealName[256];
                    if (GetRealFileName(szCurrentPath, sel, szRealName)) {
                        char szFullPath[MAX_PATH];
                        wsprintfA(szFullPath, "%s%s", szCurrentPath, szRealName);
                        ShowASCIIViewer(szFullPath);
                    } else {
                        FlashError(hActiveList);
                    }
                } else {
                    FlashError(hActiveList);
                }
            }
            break;
        }
        
        case ID_BTN_SHOW_HEX: {
            HWND hActiveList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szCurrentPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            // Vérifier qu'on n'est pas dans la liste des lecteurs
            if (strlen(szCurrentPath) == 0) {
                FlashError(hActiveList);
                break;
            }
            
            int sel = SendMessageA(hActiveList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char szItem[400];
                SendMessageA(hActiveList, LB_GETTEXT, sel, (LPARAM)szItem);
                
                // Vérifier si c'est un fichier (pas un répertoire)
                if (!strstr(szItem, "DIR")) {
                    char szRealName[256];
                    if (GetRealFileName(szCurrentPath, sel, szRealName)) {
                        char szFullPath[MAX_PATH];
                        wsprintfA(szFullPath, "%s%s", szCurrentPath, szRealName);
                        ShowHEXViewer(szFullPath);
                    } else {
                        FlashError(hActiveList);
                    }
                } else {
                    FlashError(hActiveList);
                }
            }
            break;
        }
        
        case ID_BTN_EDIT: {
            HWND hActiveList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szCurrentPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            // Vérifier qu'on n'est pas dans la liste des lecteurs
            if (strlen(szCurrentPath) == 0) {
                FlashError(hActiveList);
                break;
            }
            
            int sel = SendMessageA(hActiveList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char szItem[400];
                SendMessageA(hActiveList, LB_GETTEXT, sel, (LPARAM)szItem);
                
                // Vérifier si c'est un fichier (pas un répertoire)
                if (!strstr(szItem, "DIR")) {
                    char szRealName[256];
                    if (GetRealFileName(szCurrentPath, sel, szRealName)) {
                        char szFullPath[MAX_PATH];
                        wsprintfA(szFullPath, "%s%s", szCurrentPath, szRealName);
                        ShowASCIIEditor(szFullPath);
                    } else {
                        FlashError(hActiveList);
                    }
                } else {
                    FlashError(hActiveList);
                }
            }
            break;
        }
        
        case ID_BTN_EDIT_HEX: {
            HWND hActiveList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szCurrentPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            // Vérifier qu'on n'est pas dans la liste des lecteurs
            if (strlen(szCurrentPath) == 0) {
                FlashError(hActiveList);
                break;
            }
            
            int sel = SendMessageA(hActiveList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char szItem[400];
                SendMessageA(hActiveList, LB_GETTEXT, sel, (LPARAM)szItem);
                
                // Vérifier si c'est un fichier (pas un répertoire)
                if (!strstr(szItem, "DIR")) {
                    char szRealName[256];
                    if (GetRealFileName(szCurrentPath, sel, szRealName)) {
                        char szFullPath[MAX_PATH];
                        wsprintfA(szFullPath, "%s%s", szCurrentPath, szRealName);
                        ShowHEXEditor(szFullPath);
                    } else {
                        FlashError(hActiveList);
                    }
                } else {
                    FlashError(hActiveList);
                }
            }
            break;
        }
        
        case ID_BTN_EXECUTE: {
            int sel = SendMessageA(hActiveList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char szItem[400];
                SendMessageA(hActiveList, LB_GETTEXT, sel, (LPARAM)szItem);
                
                // Extraire le nom du fichier (avant les espaces)
                char szFileName[256];
                int i = 0;
                while (szItem[i] && szItem[i] != ' ') {
                    szFileName[i] = szItem[i];
                    i++;
                }
                szFileName[i] = '\0';
                
                // Construire le chemin complet
                char szFullPath[MAX_PATH];
                wsprintfA(szFullPath, "%s%s", szActivePath, szFileName);
                
                // Tenter d'exécuter
                if ((int)ShellExecuteA(g_hMainWindow, "open", szFullPath, NULL, NULL, SW_SHOW) <= 32) {
                    ShowCenteredMessageBox("Impossible d'exécuter ce fichier", "Erreur", MB_OK);
                }
            }
            break;
        }
        
        case ID_BTN_SETTINGS:
            ShowSettings();
            break;
            
        case ID_BTN_ARROW_RIGHT:
            // Synchroniser le répertoire du panneau gauche vers le panneau droit
            lstrcpyA(g_szRightPath, g_szLeftPath);
            if (strlen(g_szLeftPath) == 0) {
                // Si le panneau gauche affiche les lecteurs, afficher les lecteurs à droite aussi
                ListDrives(g_hListRight);
            } else {
                RefreshPanel(g_hListRight, g_szRightPath);
            }
            break;
            
        case ID_BTN_ARROW_LEFT:
            // Synchroniser le répertoire du panneau droit vers le panneau gauche
            lstrcpyA(g_szLeftPath, g_szRightPath);
            if (strlen(g_szRightPath) == 0) {
                // Si le panneau droit affiche les lecteurs, afficher les lecteurs à gauche aussi
                ListDrives(g_hListLeft);
            } else {
                RefreshPanel(g_hListLeft, g_szLeftPath);
            }
            break;
            
        case ID_BTN_QUIT:
            // Quitter directement sans confirmation
            PostQuitMessage(0);
            break;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Barres de statut pour afficher les chemins
            g_hStatusLeft = CreateWindowA("STATIC", g_szLeftPath,
                WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
                10, 500, 335, 20, hwnd, NULL, NULL, NULL);
                
            g_hStatusRight = CreateWindowA("STATIC", g_szRightPath,
                WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
                455, 500, 335, 20, hwnd, NULL, NULL, NULL);
            
            // Boutons flèches pour synchroniser les répertoires
            g_hArrowRight = CreateWindowA("BUTTON", ">",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                350, 500, 20, 20, hwnd, (HMENU)ID_BTN_ARROW_RIGHT, NULL, NULL);
                
            g_hArrowLeft = CreateWindowA("BUTTON", "<",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                430, 500, 20, 20, hwnd, (HMENU)ID_BTN_ARROW_LEFT, NULL, NULL);
            
            // Panneau gauche (marges de 10px)
            g_hListLeft = CreateWindowA("LISTBOX", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_MULTIPLESEL,
                10, 10, 335, 480, hwnd, (HMENU)ID_LISTBOX_LEFT, NULL, NULL);
                
            // Panneau droit (marges de 10px)
            g_hListRight = CreateWindowA("LISTBOX", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_MULTIPLESEL,
                455, 10, 335, 480, hwnd, (HMENU)ID_LISTBOX_RIGHT, NULL, NULL);
            
            // Boutons de commandes au centre
            const char* btnTexts[] = {
                "Home", "Parent", "Invert", "Clear", 
                "Copy", "Move", "Delete", "Rename",
                "Makedir", "Show Asc", "Show HEX", "Edit",
                "Edit Hex", "Execute"
            };
            
            int btnIds[] = {
                ID_BTN_CEPC, ID_BTN_PARENT, ID_BTN_INVERT, ID_BTN_CLEAR,
                ID_BTN_COPY, ID_BTN_MOVE, ID_BTN_DELETE, ID_BTN_RENAME,
                ID_BTN_MAKEDIR, ID_BTN_SHOW_ASC, ID_BTN_SHOW_HEX, ID_BTN_EDIT,
                ID_BTN_EDIT_HEX, ID_BTN_EXECUTE
            };
            
            for (int i = 0; i < 14; i++) {
                CreateWindowA("BUTTON", btnTexts[i],
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    355, 10 + (i * 35), 90, 30, hwnd, (HMENU)btnIds[i], NULL, NULL);
            }
            
            // Boutons Settings et Quit sous les barres de statut (alignés à droite)
            CreateWindowA("BUTTON", "Settings",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                580, 530, 80, 25, hwnd, (HMENU)ID_BTN_SETTINGS, NULL, NULL);
                
            CreateWindowA("BUTTON", "Quit",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                670, 530, 60, 25, hwnd, (HMENU)ID_BTN_QUIT, NULL, NULL);
            
            // Initialiser les panneaux
            RefreshPanel(g_hListLeft, g_szLeftPath);
            RefreshPanel(g_hListRight, g_szRightPath);
            
            // Appliquer les polices aux contrôles principaux
            if (g_hFontMono) {
                // Police monospace pour les listes de fichiers
                SendMessageA(g_hListLeft, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
                SendMessageA(g_hListRight, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
            }
            if (g_hFontUTF8) {
                // Police UTF-8 pour les barres de statut
                SendMessageA(g_hStatusLeft, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                SendMessageA(g_hStatusRight, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                
                // Police UTF-8 pour les boutons flèches
                SendMessageA(g_hArrowRight, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                SendMessageA(g_hArrowLeft, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
            }
            
            // Installer le sous-classement pour la sélection Windows-style
            g_oldListProcLeft = (WNDPROC)SetWindowLongPtrA(g_hListLeft, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);
            g_oldListProcRight = (WNDPROC)SetWindowLongPtrA(g_hListRight, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);
            
            break;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            
            if (wmEvent == BN_CLICKED) {
                ExecuteCommand(wmId);
            } else if (wmEvent == LBN_DBLCLK) {
                HWND hList = (HWND)lParam;
                int sel = SendMessageA(hList, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    char szItem[400];
                    SendMessageA(hList, LB_GETTEXT, sel, (LPARAM)szItem);
                    
                    // Détermine quel panneau
                    int panel = (hList == g_hListLeft) ? 0 : 1;
                    char* szPath = (panel == 0) ? g_szLeftPath : g_szRightPath;
                    g_nActivePanel = panel;
                    
                    // Extraire le nom du fichier (35 premiers caractères max, avant la taille)
                    char szName[256];
                    int i = 0;
                    
                    // Copier jusqu'à 35 caractères ou jusqu'à voir "..." qui indique une troncature
                    while (i < 35 && szItem[i]) {
                        if (szItem[i] == '.' && szItem[i+1] == '.' && szItem[i+2] == '.') {
                            // Nom tronqué, on ne peut pas naviguer
                            szName[0] = '\0';
                            break;
                        }
                        szName[i] = szItem[i];
                        i++;
                    }
                    
                    // Enlever les espaces à la fin
                    while (i > 0 && szName[i-1] == ' ') {
                        i--;
                    }
                    szName[i] = '\0';
                    
                    // Vérifier si c'est un répertoire (contient "DRIVE" à la fin)
                    if (szName[0] == '\0') {
                        // Nom tronqué, impossible de naviguer
                        ShowCenteredMessageBox("Impossible de naviguer : nom de fichier tronqué", "Erreur", MB_OK | MB_ICONWARNING);
                    } else if (simple_strstr(szItem, "DRIVE")) {
                        // Lecteur (C:, D:, etc.)
                        if (szItem[0] >= 'A' && szItem[0] <= 'Z' && szItem[1] == ':') {
                            wsprintfA(szPath, "%c:\\", szItem[0]);
                            RefreshPanel(hList, szPath);
                        }
                    } else if (simple_strstr(szItem, "DIR")) {
                        // Répertoire normal
                        lstrcatA(szPath, szName);
                        lstrcatA(szPath, "\\");
                        RefreshPanel(hList, szPath);
                    }
                }
            } else if (wmEvent == LBN_SELCHANGE) {
                // Changer le panneau actif
                HWND hList = (HWND)lParam;
                g_nActivePanel = (hList == g_hListLeft) ? 0 : 1;
                
                // Forcer le redraw des deux panneaux pour mettre à jour les couleurs
                InvalidateRect(g_hListLeft, NULL, TRUE);
                InvalidateRect(g_hListRight, NULL, TRUE);
            } else if (wmEvent == LBN_SETFOCUS) {
                // Activer le panneau même sur un simple clic (focus)
                HWND hList = (HWND)lParam;
                g_nActivePanel = (hList == g_hListLeft) ? 0 : 1;
                
                // Forcer le redraw des deux panneaux pour mettre à jour les couleurs
                InvalidateRect(g_hListLeft, NULL, TRUE);
                InvalidateRect(g_hListRight, NULL, TRUE);
            }
            break;
        }
        
        case WM_CTLCOLORLISTBOX: {
            HWND hListBox = (HWND)lParam;
            HDC hdc = (HDC)wParam;
            
            // Déterminer si c'est le panneau actif ou inactif
            BOOL isActive = FALSE;
            if (hListBox == g_hListLeft && g_nActivePanel == 0) isActive = TRUE;
            if (hListBox == g_hListRight && g_nActivePanel == 1) isActive = TRUE;
            
            if (isActive) {
                SetBkColor(hdc, RGB(102, 136, 187));
                return (LRESULT)g_hBrushActive;
            } else {
                SetBkColor(hdc, RGB(170, 170, 170));
                return (LRESULT)g_hBrushInactive;
            }
        }
        
        case WM_CLOSE:
            // Quitter directement sans confirmation
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Créer les brushes pour les couleurs des panneaux
    g_hBrushActive = CreateSolidBrush(RGB(102, 136, 187));
    g_hBrushInactive = CreateSolidBrush(RGB(170, 170, 170));
    
    // Charger les préférences avant de créer les polices
    LoadSettings();
    
    // Créer les polices basées sur les préférences
    g_hFontUTF8 = CreateFontA(-g_settings.fontSizeSettings, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, g_settings.fontSettings);
    
    g_hFontMono = CreateFontA(-g_settings.fontSizePanels, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, g_settings.fontPanels);
    
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FM";
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    
    RegisterClassA(&wc);
    
    // Calculer la taille exacte de fenêtre pour une zone client de 800x600
    RECT rect = {0, 0, 800, 600};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rect, style, FALSE);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    
    // Centrer la fenêtre sur l'écran
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int centerX = (screenWidth - windowWidth) / 2;
    int centerY = (screenHeight - windowHeight) / 2;
    
    g_hMainWindow = CreateWindowA("FM", "FileMaster 1.0", style,
        centerX, centerY, windowWidth, windowHeight, NULL, NULL, hInstance, NULL);
    
    ShowWindow(g_hMainWindow, SW_SHOW);
    
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        DispatchMessageA(&msg);
    }
    
    // Nettoyer les brushes et les polices
    if (g_hBrushActive) DeleteObject(g_hBrushActive);
    if (g_hBrushInactive) DeleteObject(g_hBrushInactive);
    if (g_hFontUTF8) DeleteObject(g_hFontUTF8);
    if (g_hFontMono) DeleteObject(g_hFontMono);
    
    ExitProcess(0);
}
