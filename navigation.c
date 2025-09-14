// navigation.c - Module de navigation et gestion des panneaux
// ============================================================

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "prototypes.h"

// Variables externes
extern HWND g_hListLeft, g_hListRight;
extern HWND g_hStatusLeft, g_hStatusRight;
extern char g_szLeftPath[MAX_PATH], g_szRightPath[MAX_PATH];
extern AppSettings g_settings;

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