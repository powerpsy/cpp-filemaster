#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <shellapi.h>

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

// Simple strchr implementation
char* simple_strchr(const char* str, int c) {
    while (*str) {
        if (*str == c) return (char*)str;
        str++;
    }
    return NULL;
}

// Simple atoi implementation
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

// Prototypes
void RefreshPanel(HWND hList, const char* szPath);
void ListDrives(HWND hList);
void GoToParent(int panel);
void ExecuteCommand(int cmdId);
void ShowSettings();
void LoadSettings();
void SaveSettings();
void FlashError(HWND hwnd); // Effet de clignotement pour signaler une erreur
int ShowCenteredMessageBox(const char* text, const char* caption, UINT type);
HWND CreateCenteredWindow(const char* className, const char* windowName, DWORD style, int width, int height, HWND parent, HINSTANCE hInstance);

// Prototypes pour viewers/éditeurs
void ShowASCIIViewer(const char* filename);
void ShowHEXViewer(const char* filename);
void ShowASCIIEditor(const char* filename);
void ShowHEXEditor(const char* filename);
LRESULT CALLBACK ViewerWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
char* FormatASCIIData(const BYTE* data, DWORD size);

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
    
    // Sauvegarder les couleurs originales
    COLORREF originalBg = GetSysColor(COLOR_WINDOW);
    
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

// Structure pour les préférences
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

// Formate la taille en bytes/KB/MB selon le format: toujours 5 caractères (x'xxx ou xxx.x ou xx.xx ou x.xxx)
void FormatFileSize(DWORD sizeLow, DWORD sizeHigh, char* buffer) {
    // Combiner les parties haute et basse pour obtenir la taille complète
    unsigned long long totalSize = ((unsigned long long)sizeHigh << 32) | sizeLow;
    
    if (totalSize >= 1024ULL * 1024ULL * 1024ULL * 1024ULL) {
        // To - format: toujours 5 caractères
        unsigned long long tb_x1000 = (totalSize * 1000) / (1024ULL * 1024ULL * 1024ULL * 1024ULL);
        if (tb_x1000 >= 10000) {        // >= 10.000 To -> "x'xxx"
            wsprintfA(buffer, "%5lu To", (unsigned long)(tb_x1000 / 1000));
        } else if (tb_x1000 >= 1000) {  // >= 1.000 To -> "x.xxx"
            wsprintfA(buffer, "%lu.%03lu To", (unsigned long)(tb_x1000 / 1000), (unsigned long)(tb_x1000 % 1000));
        } else if (tb_x1000 >= 100) {   // >= 0.100 To -> "xx.xx"
            wsprintfA(buffer, "%02lu.%02lu To", (unsigned long)(tb_x1000 / 100), (unsigned long)((tb_x1000 % 100) / 10));
        } else {                        // < 0.100 To -> "xxx.x"
            wsprintfA(buffer, "%03lu.%lu To", (unsigned long)(tb_x1000 / 10), (unsigned long)(tb_x1000 % 10));
        }
    } else if (totalSize >= 1024ULL * 1024ULL * 1024ULL) {
        // Go - format: toujours 5 caractères
        unsigned long long gb_x1000 = (totalSize * 1000) / (1024ULL * 1024ULL * 1024ULL);
        if (gb_x1000 >= 10000) {        // >= 10.000 Go -> "x'xxx"
            wsprintfA(buffer, "%5lu Go", (unsigned long)(gb_x1000 / 1000));
        } else if (gb_x1000 >= 1000) {  // >= 1.000 Go -> "x.xxx"
            wsprintfA(buffer, "%lu.%03lu Go", (unsigned long)(gb_x1000 / 1000), (unsigned long)(gb_x1000 % 1000));
        } else if (gb_x1000 >= 100) {   // >= 0.100 Go -> "xx.xx"
            wsprintfA(buffer, "%02lu.%02lu Go", (unsigned long)(gb_x1000 / 100), (unsigned long)((gb_x1000 % 100) / 10));
        } else {                        // < 0.100 Go -> "xxx.x"
            wsprintfA(buffer, "%03lu.%lu Go", (unsigned long)(gb_x1000 / 10), (unsigned long)(gb_x1000 % 10));
        }
    } else if (totalSize >= 1024ULL * 1024ULL) {
        // Mo - format: toujours 5 caractères
        unsigned long long mb_x1000 = (totalSize * 1000) / (1024ULL * 1024ULL);
        if (mb_x1000 >= 10000) {        // >= 10.000 Mo -> "x'xxx"
            wsprintfA(buffer, "%5lu Mo", (unsigned long)(mb_x1000 / 1000));
        } else if (mb_x1000 >= 1000) {  // >= 1.000 Mo -> "x.xxx"
            wsprintfA(buffer, "%lu.%03lu Mo", (unsigned long)(mb_x1000 / 1000), (unsigned long)(mb_x1000 % 1000));
        } else if (mb_x1000 >= 100) {   // >= 0.100 Mo -> "xx.xx"
            wsprintfA(buffer, "%02lu.%02lu Mo", (unsigned long)(mb_x1000 / 100), (unsigned long)((mb_x1000 % 100) / 10));
        } else {                        // < 0.100 Mo -> "xxx.x"
            wsprintfA(buffer, "%03lu.%lu Mo", (unsigned long)(mb_x1000 / 10), (unsigned long)(mb_x1000 % 10));
        }
    } else if (totalSize >= 1024ULL) {
        // ko - format: toujours 5 caractères
        unsigned long long kb_x1000 = (totalSize * 1000) / 1024ULL;
        if (kb_x1000 >= 10000) {        // >= 10.000 ko -> "x'xxx"
            wsprintfA(buffer, "%5lu ko", (unsigned long)(kb_x1000 / 1000));
        } else if (kb_x1000 >= 1000) {  // >= 1.000 ko -> "x.xxx"
            wsprintfA(buffer, "%lu.%03lu ko", (unsigned long)(kb_x1000 / 1000), (unsigned long)(kb_x1000 % 1000));
        } else if (kb_x1000 >= 100) {   // >= 0.100 ko -> "xx.xx"
            wsprintfA(buffer, "%02lu.%02lu ko", (unsigned long)(kb_x1000 / 100), (unsigned long)((kb_x1000 % 100) / 10));
        } else {                        // < 0.100 ko -> "xxx.x"
            wsprintfA(buffer, "%03lu.%lu ko", (unsigned long)(kb_x1000 / 10), (unsigned long)(kb_x1000 % 10));
        }
    } else {
        // o - format: toujours 5 caractères
        if (totalSize >= 10000) {
            wsprintfA(buffer, "%5lu  o", (unsigned long)totalSize);
        } else {
            wsprintfA(buffer, "%5lu  o", (unsigned long)totalSize);
        }
    }
}

// Fonctions de gestion des fichiers pour les viewers/éditeurs
BOOL LoadFileData(const char* filename, BYTE** data, DWORD* size) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        char errorMsg[512];
        switch (error) {
            case ERROR_FILE_NOT_FOUND:
                wsprintfA(errorMsg, "Fichier non trouvé :\n%s", filename);
                break;
            case ERROR_ACCESS_DENIED:
                wsprintfA(errorMsg, "Accès refusé :\n%s\n\nLe fichier est peut-être utilisé par un autre programme ou vous n'avez pas les permissions nécessaires.", filename);
                break;
            case ERROR_SHARING_VIOLATION:
                wsprintfA(errorMsg, "Fichier en cours d'utilisation :\n%s\n\nLe fichier est ouvert dans un autre programme.", filename);
                break;
            case ERROR_PATH_NOT_FOUND:
                wsprintfA(errorMsg, "Chemin non trouvé :\n%s", filename);
                break;
            default:
                wsprintfA(errorMsg, "Impossible d'ouvrir le fichier :\n%s\n\nCode d'erreur Windows : %d", filename, error);
                break;
        }
        ShowCenteredMessageBox(errorMsg, "Erreur d'ouverture", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    *size = GetFileSize(hFile, NULL);
    if (*size == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        ShowCenteredMessageBox("Impossible de déterminer la taille du fichier", "Erreur", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    *data = (BYTE*)HeapAlloc(GetProcessHeap(), 0, *size + 1);
    if (!*data) {
        CloseHandle(hFile);
        ShowCenteredMessageBox("Mémoire insuffisante pour charger le fichier", "Erreur", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    DWORD bytesRead;
    if (!ReadFile(hFile, *data, *size, &bytesRead, NULL) || bytesRead != *size) {
        HeapFree(GetProcessHeap(), 0, *data);
        *data = NULL;
        CloseHandle(hFile);
        ShowCenteredMessageBox("Erreur lors de la lecture du fichier", "Erreur", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    (*data)[*size] = 0; // Null terminator pour l'affichage ASCII
    CloseHandle(hFile);
    return TRUE;
}

// Fonction pour formater les données ASCII en remplaçant les caractères non-imprimables
char* FormatASCIIData(const BYTE* data, DWORD size) {
    char* asciiText = (char*)HeapAlloc(GetProcessHeap(), 0, size + 1);
    if (!asciiText) return NULL;
    
    for (DWORD i = 0; i < size; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            // Caractère imprimable
            asciiText[i] = data[i];
        } else if (data[i] == '\r' || data[i] == '\n' || data[i] == '\t') {
            // Conserver les retours chariot, nouvelles lignes et tabulations
            asciiText[i] = data[i];
        } else {
            // Remplacer les autres caractères non-imprimables par un point
            asciiText[i] = '.';
        }
    }
    asciiText[size] = '\0';
    return asciiText;
}

BOOL SaveFileData(const char* filename, const BYTE* data, DWORD size) {
    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    
    DWORD bytesWritten;
    BOOL result = WriteFile(hFile, data, size, &bytesWritten, NULL) && bytesWritten == size;
    CloseHandle(hFile);
    return result;
}

// Fonction pour formater les données en HEX avec 8 colonnes
void FormatHexData(const BYTE* data, DWORD size, char** output) {
    // Calcul de la taille nécessaire: chaque ligne = 32 bytes = 8 colonnes de 4 bytes
    // Format: "XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX  ................................\r\n"
    // = 8*8 + 7 + 2 + 32 + 2 = 64+7+2+32+2 = 107 caractères par ligne
    DWORD lines = (size + 31) / 32;
    DWORD outputSize = lines * 107 + 1;
    
    *output = (char*)HeapAlloc(GetProcessHeap(), 0, outputSize);
    if (!*output) return;
    
    char* ptr = *output;
    for (DWORD i = 0; i < size; i += 32) {
        // 8 colonnes de 4 bytes en HEX
        for (int col = 0; col < 8; col++) {
            for (int j = 0; j < 4; j++) {
                DWORD idx = i + col * 4 + j;
                if (idx < size) {
                    wsprintfA(ptr, "%02X", data[idx]);
                    ptr += 2;
                } else {
                    lstrcpyA(ptr, "  ");
                    ptr += 2;
                }
            }
            if (col < 7) {
                *ptr++ = ' ';
            }
        }
        
        // Séparateur
        *ptr++ = ' ';
        *ptr++ = ' ';
        
        // Affichage ASCII des 32 caractères
        for (DWORD j = 0; j < 32; j++) {
            DWORD idx = i + j;
            if (idx < size) {
                char c = data[idx];
                *ptr++ = (c >= 32 && c <= 126) ? c : '.';
            } else {
                *ptr++ = ' ';
            }
        }
        
        *ptr++ = '\r';
        *ptr++ = '\n';
    }
    *ptr = 0;
}

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

// Dialogue de renommage
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

// Procédure de fenêtre pour la création de répertoire
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

// Affiche la boîte de dialogue de renommage
BOOL ShowRenameDialog(const char* currentName, char* newName) {
    // Préparer le nom actuel
    lstrcpyA(g_renameResult, currentName);
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

// Affiche la boîte de dialogue de création de répertoire
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

// Dialogue de conflit pour la copie de fichiers
LRESULT CALLBACK ConflictDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Titre et message
            CreateWindowA("STATIC", "Le fichier existe déjà. Que voulez-vous faire ?",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                10, 10, 480, 20, hwnd, NULL, NULL, NULL);
            
            // Informations fichier source
            HWND hSourceInfo = CreateWindowA("STATIC", g_sourceInfo,
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 40, 480, 20, hwnd, NULL, NULL, NULL);
                
            // Informations fichier destination  
            HWND hDestInfo = CreateWindowA("STATIC", g_destInfo,
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 65, 480, 20, hwnd, NULL, NULL, NULL);
            
            // Boutons d'action (centrés)
            CreateWindowA("BUTTON", "Remplacer",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                125, 100, 80, 30, hwnd, (HMENU)ID_COPY_REPLACE, NULL, NULL);
                
            CreateWindowA("BUTTON", "Passer",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                215, 100, 80, 30, hwnd, (HMENU)ID_COPY_SKIP, NULL, NULL);
                
            CreateWindowA("BUTTON", "Annuler tout",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                305, 100, 80, 30, hwnd, (HMENU)ID_COPY_CANCEL, NULL, NULL);
            
            // Checkbox "Appliquer à tous" (centrée)
            CreateWindowA("BUTTON", "Appliquer ce choix à tous les fichiers",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                110, 140, 300, 20, hwnd, (HMENU)ID_COPY_APPLY_ALL, NULL, NULL);
            
            // Appliquer police UTF-8
            if (g_hFontUTF8) {
                for (int id = ID_COPY_REPLACE; id <= ID_COPY_APPLY_ALL; id++) {
                    HWND hCtrl = GetDlgItem(hwnd, id);
                    if (hCtrl) SendMessageA(hCtrl, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                }
                // Appliquer aussi aux textes d'information
                SendMessageA(hSourceInfo, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
                SendMessageA(hDestInfo, WM_SETFONT, (WPARAM)g_hFontUTF8, TRUE);
            }
            break;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId >= ID_COPY_REPLACE && wmId <= ID_COPY_CANCEL) {
                // Récupérer l'état de la checkbox
                g_copyApplyAll = SendMessageA(GetDlgItem(hwnd, ID_COPY_APPLY_ALL), BM_GETCHECK, 0, 0);
                
                // Définir le choix
                g_copyChoice = wmId - ID_COPY_REPLACE + 1; // 1=remplacer, 2=passer, 3=annuler
                
                // Fermer la boîte de dialogue
                DestroyWindow(hwnd);
            }
            break;
        }
        
        case WM_KEYDOWN: {
            if (wParam == VK_RETURN) {
                // RETURN = Remplacer
                g_copyApplyAll = SendMessageA(GetDlgItem(hwnd, ID_COPY_APPLY_ALL), BM_GETCHECK, 0, 0);
                g_copyChoice = 1; // Remplacer
                DestroyWindow(hwnd);
                return 0;
            } else if (wParam == VK_ESCAPE) {
                // ESC = Annuler tout
                g_copyChoice = 3; // Annuler
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        
        case WM_CLOSE:
            g_copyChoice = 3; // Annuler
            DestroyWindow(hwnd);
            break;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// Affiche la boîte de dialogue de conflit
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

// Formate les informations d'un fichier pour l'affichage
void FormatFileInfo(const char* filePath, char* buffer, int bufferSize, const char* label) {
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(filePath, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        // Extraire le nom de fichier
        const char* fileName = filePath;
        const char* lastSlash = filePath;
        while (*lastSlash) {
            if (*lastSlash == '\\') fileName = lastSlash + 1;
            lastSlash++;
        }
        
        // Formater la taille
        char szSize[20];
        FormatFileSize(findData.nFileSizeLow, findData.nFileSizeHigh, szSize);
        
        // Formater la date
        SYSTEMTIME st;
        FILETIME ft;
        FileTimeToLocalFileTime(&findData.ftLastWriteTime, &ft);
        FileTimeToSystemTime(&ft, &st);
        
        wsprintfA(buffer, "%s %s %02d/%02d/%04d %02d:%02d (%s)", 
                 fileName, szSize, st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, label);
        
        FindClose(hFind);
    } else {
        wsprintfA(buffer, "%s - [Erreur lecture] (%s)", filePath, label);
    }
}

int ShowConflictDialog(const char* sourceFile, const char* destFile) {
    // Réinitialiser si ce n'est pas "appliquer à tous"
    if (!g_copyApplyAll) {
        g_copyChoice = 0;
    }
    
    // Si on a déjà un choix "appliquer à tous", le retourner
    if (g_copyApplyAll && g_copyChoice != 0) {
        return g_copyChoice;
    }
    
    // Préparer les informations des fichiers
    FormatFileInfo(sourceFile, g_sourceInfo, sizeof(g_sourceInfo), "source");
    FormatFileInfo(destFile, g_destInfo, sizeof(g_destInfo), "destination");
    
    // Créer et afficher la boîte de dialogue modale
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = ConflictDialogProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "ConflictDlg";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    
    // Enregistrer la classe seulement si elle n'existe pas déjà
    if (!GetClassInfoA(GetModuleHandleA(NULL), "ConflictDlg", &wc)) {
        RegisterClassA(&wc);
    }
    
    HWND hDlg = CreateCenteredWindow("ConflictDlg", "Conflit de fichier",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 520, 200, g_hMainWindow, GetModuleHandleA(NULL));
    
    // Rendre modal
    EnableWindow(g_hMainWindow, FALSE);
    ShowWindow(hDlg, SW_SHOW);
    SetForegroundWindow(hDlg);
    
    // Boucle de messages pour la modalité
    MSG msg;
    while (IsWindow(hDlg)) {
        if (GetMessageA(&msg, NULL, 0, 0)) {
            if (msg.hwnd == hDlg || IsChild(hDlg, msg.hwnd)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } else {
                // Messages pour d'autres fenêtres - les ignorer pour maintenir la modalité
                if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) {
                    continue; // Ignorer les messages clavier pour les autres fenêtres
                }
                if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST) {
                    continue; // Ignorer les messages souris pour les autres fenêtres
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        } else {
            break; // GetMessage a retourné FALSE
        }
    }
    
    // Réactiver la fenêtre principale
    EnableWindow(g_hMainWindow, TRUE);
    SetForegroundWindow(g_hMainWindow);
    
    return g_copyChoice;
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

// Fonctions pour les viewers/éditeurs
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
