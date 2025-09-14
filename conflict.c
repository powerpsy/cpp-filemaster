// conflict.c - Gestion des conflits de fichiers
#include <windows.h>
#include "prototypes.h"

// Variables globales externes (définies dans main.c)
extern HWND g_hMainWindow;
extern HFONT g_hFontUTF8;
extern int g_copyChoice;
extern BOOL g_copyApplyAll;
extern char g_sourceInfo[512];
extern char g_destInfo[512];

// Identifiants pour la boîte de dialogue de conflit
#define ID_COPY_REPLACE     4001
#define ID_COPY_SKIP        4002
#define ID_COPY_CANCEL      4003
#define ID_COPY_APPLY_ALL   4004

// Procédure de fenêtre pour la boîte de dialogue de conflit
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

// Affiche la boîte de dialogue de conflit
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