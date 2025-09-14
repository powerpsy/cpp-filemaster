#include <windows.h>
#include <stdlib.h>
#include <shellapi.h>
#include "prototypes.h"
#include "constants.h"

// Variables externes nécessaires
extern HWND g_hListLeft, g_hListRight, g_hMainWindow;
extern char g_szLeftPath[MAX_PATH], g_szRightPath[MAX_PATH];
extern int g_nActivePanel;
extern AppSettings g_settings;

// Variables pour les conflits (défini dans fileops.c)
extern int g_copyChoice;
extern BOOL g_copyApplyAll;

// Gestion des commandes principales
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
                    
                    // Copier le fichier avec gestion des conflits
                    if (CopyFileA(szSourceFile, szDestFile, FALSE)) {
                        copiedFiles++;
                    } else {
                        skippedFiles++;
                    }
                }
            }
            
            // Rafraîchir le panneau de destination
            HWND hDestList = (g_nActivePanel == 0) ? g_hListRight : g_hListLeft;
            RefreshPanel(hDestList, szDestPath);
            
            // Message de confirmation
            if (!operationCancelled) {
                char szMessage[256];
                if (skippedFiles > 0) {
                    wsprintfA(szMessage, "%d fichier(s) copié(s), %d fichier(s) ignoré(s)", copiedFiles, skippedFiles);
                } else {
                    wsprintfA(szMessage, "%d fichier(s) copié(s)", copiedFiles);
                }
                ShowCenteredMessageBox(szMessage, "Copie terminée", MB_OK | MB_ICONINFORMATION);
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
                    
                    // Déplacer le fichier avec gestion des conflits
                    if (CopyFileA(szSourceFile, szDestFile, FALSE)) {
                        if (DeleteFileA(szSourceFile)) {
                            movedFiles++;
                        } else {
                            DeleteFileA(szDestFile); // Nettoyer en cas d'échec
                            skippedFiles++;
                        }
                    } else {
                        skippedFiles++;
                    }
                }
            }
            
            // Rafraîchir les deux panneaux
            RefreshPanel(hSourceList, szSourcePath);
            HWND hDestList = (g_nActivePanel == 0) ? g_hListRight : g_hListLeft;
            RefreshPanel(hDestList, szDestPath);
            
            // Message de confirmation
            if (!operationCancelled) {
                char szMessage[256];
                if (skippedFiles > 0) {
                    wsprintfA(szMessage, "%d fichier(s) déplacé(s), %d fichier(s) ignoré(s)", movedFiles, skippedFiles);
                } else {
                    wsprintfA(szMessage, "%d fichier(s) déplacé(s)", movedFiles);
                }
                ShowCenteredMessageBox(szMessage, "Déplacement terminé", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
            
        case ID_BTN_DELETE: {
            // Supprimer les fichiers sélectionnés avec confirmation
            HWND hList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            int count = SendMessageA(hList, LB_GETCOUNT, 0, 0);
            int selectedCount = 0;
            
            // Compter les fichiers sélectionnés
            for (int i = 0; i < count; i++) {
                int isSelected = SendMessageA(hList, LB_GETSEL, i, 0);
                if (isSelected) {
                    // Récupérer le nom réel du fichier (pas l'affichage tronqué)
                    char szRealFileName[MAX_PATH];
                    if (GetRealFileName(szPath, i, szRealFileName)) {
                        // Ignorer le dossier parent ".."
                        if (lstrcmpA(szRealFileName, "..") != 0) {
                            selectedCount++;
                        }
                    }
                }
            }
            
            if (selectedCount == 0) break;
            
            // Demander confirmation
            char szMessage[256];
            wsprintfA(szMessage, "Êtes-vous sûr de vouloir supprimer %d fichier(s) ?", selectedCount);
            int result = ShowCenteredMessageBox(szMessage, "Confirmation", MB_YESNO | MB_ICONQUESTION);
            
            if (result == IDYES) {
                int deletedFiles = 0;
                for (int i = 0; i < count; i++) {
                    int isSelected = SendMessageA(hList, LB_GETSEL, i, 0);
                    if (isSelected) {
                        // Récupérer le nom réel du fichier
                        char szRealFileName[MAX_PATH];
                        if (!GetRealFileName(szPath, i, szRealFileName)) {
                            continue;
                        }
                        
                        // Ignorer le dossier parent ".."
                        if (lstrcmpA(szRealFileName, "..") == 0) continue;
                        
                        // Construire le chemin complet
                        char szFullPath[MAX_PATH];
                        wsprintfA(szFullPath, "%s%s", szPath, szRealFileName);
                        
                        // Supprimer le fichier
                        if (DeleteFileA(szFullPath)) {
                            deletedFiles++;
                        }
                    }
                }
                
                // Rafraîchir le panneau
                RefreshPanel(hList, szPath);
                
                // Message de confirmation
                wsprintfA(szMessage, "%d fichier(s) supprimé(s)", deletedFiles);
                ShowCenteredMessageBox(szMessage, "Suppression terminée", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
            
        case ID_BTN_RENAME:
            // Renommer un fichier
            // Cette fonctionnalité nécessite une interface complexe, simplifiée pour l'instant
            ShowCenteredMessageBox("Fonctionnalité en développement", "Renommer", MB_OK | MB_ICONINFORMATION);
            break;
            
        case ID_BTN_MAKEDIR:
            // Créer un nouveau dossier
            // Cette fonctionnalité nécessite une interface complexe, simplifiée pour l'instant  
            ShowCenteredMessageBox("Fonctionnalité en développement", "Créer dossier", MB_OK | MB_ICONINFORMATION);
            break;
            
        case ID_BTN_SHOW_ASC: {
            // Afficher le fichier en ASCII
            HWND hList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            int sel = SendMessageA(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char szRealFileName[MAX_PATH];
                if (GetRealFileName(szPath, sel, szRealFileName) && 
                    lstrcmpA(szRealFileName, "..") != 0) {
                    
                    char szFullPath[MAX_PATH];
                    wsprintfA(szFullPath, "%s%s", szPath, szRealFileName);
                    
                    // Vérifier que ce n'est pas un dossier
                    DWORD attrs = GetFileAttributesA(szFullPath);
                    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        ShowASCIIViewer(szFullPath);
                    } else {
                        ShowCenteredMessageBox("Veuillez sélectionner un fichier", "Viewer ASCII", MB_OK | MB_ICONWARNING);
                    }
                }
            } else {
                ShowCenteredMessageBox("Aucun fichier sélectionné", "Viewer ASCII", MB_OK | MB_ICONWARNING);
            }
            break;
        }
            
        case ID_BTN_SHOW_HEX: {
            // Afficher le fichier en HEX
            HWND hList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            int sel = SendMessageA(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char szRealFileName[MAX_PATH];
                if (GetRealFileName(szPath, sel, szRealFileName) && 
                    lstrcmpA(szRealFileName, "..") != 0) {
                    
                    char szFullPath[MAX_PATH];
                    wsprintfA(szFullPath, "%s%s", szPath, szRealFileName);
                    
                    // Vérifier que ce n'est pas un dossier
                    DWORD attrs = GetFileAttributesA(szFullPath);
                    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        ShowHEXViewer(szFullPath);
                    } else {
                        ShowCenteredMessageBox("Veuillez sélectionner un fichier", "Viewer HEX", MB_OK | MB_ICONWARNING);
                    }
                }
            } else {
                ShowCenteredMessageBox("Aucun fichier sélectionné", "Viewer HEX", MB_OK | MB_ICONWARNING);
            }
            break;
        }
            
        case ID_BTN_EDIT: {
            // Éditer le fichier en texte ASCII
            HWND hList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            int sel = SendMessageA(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char szRealFileName[MAX_PATH];
                if (GetRealFileName(szPath, sel, szRealFileName) && 
                    lstrcmpA(szRealFileName, "..") != 0) {
                    
                    char szFullPath[MAX_PATH];
                    wsprintfA(szFullPath, "%s%s", szPath, szRealFileName);
                    
                    // Vérifier que ce n'est pas un dossier
                    DWORD attrs = GetFileAttributesA(szFullPath);
                    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        ShowASCIIEditor(szFullPath);
                    } else {
                        ShowCenteredMessageBox("Veuillez sélectionner un fichier", "Éditeur ASCII", MB_OK | MB_ICONWARNING);
                    }
                }
            } else {
                ShowCenteredMessageBox("Aucun fichier sélectionné", "Éditeur ASCII", MB_OK | MB_ICONWARNING);
            }
            break;
        }
            
        case ID_BTN_EDIT_HEX: {
            // Éditer le fichier en HEX
            HWND hList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            int sel = SendMessageA(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char szRealFileName[MAX_PATH];
                if (GetRealFileName(szPath, sel, szRealFileName) && 
                    lstrcmpA(szRealFileName, "..") != 0) {
                    
                    char szFullPath[MAX_PATH];
                    wsprintfA(szFullPath, "%s%s", szPath, szRealFileName);
                    
                    // Vérifier que ce n'est pas un dossier
                    DWORD attrs = GetFileAttributesA(szFullPath);
                    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        ShowHEXEditor(szFullPath);
                    } else {
                        ShowCenteredMessageBox("Veuillez sélectionner un fichier", "Éditeur HEX", MB_OK | MB_ICONWARNING);
                    }
                }
            } else {
                ShowCenteredMessageBox("Aucun fichier sélectionné", "Éditeur HEX", MB_OK | MB_ICONWARNING);
            }
            break;
        }
            
        case ID_BTN_EXECUTE: {
            // Exécuter le fichier sélectionné
            HWND hList = (g_nActivePanel == 0) ? g_hListLeft : g_hListRight;
            char* szPath = (g_nActivePanel == 0) ? g_szLeftPath : g_szRightPath;
            
            int sel = SendMessageA(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                // Récupérer le nom réel du fichier
                char szRealFileName[MAX_PATH];
                if (GetRealFileName(szPath, sel, szRealFileName) && 
                    lstrcmpA(szRealFileName, "..") != 0) {
                    
                    char szFullPath[MAX_PATH];
                    wsprintfA(szFullPath, "%s%s", szPath, szRealFileName);
                    
                    // Vérifier que ce n'est pas un dossier
                    DWORD attrs = GetFileAttributesA(szFullPath);
                    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        if ((int)ShellExecuteA(g_hMainWindow, "open", szFullPath, NULL, NULL, SW_SHOW) <= 32) {
                            ShowCenteredMessageBox("Impossible d'exécuter le fichier", "Erreur", MB_OK | MB_ICONERROR);
                        }
                    }
                }
            }
            break;
        }
            
        case ID_BTN_ARROW_RIGHT: {
            // Synchroniser le panneau droit avec le gauche
            if (strlen(g_szLeftPath) > 0) {
                lstrcpyA(g_szRightPath, g_szLeftPath);
                RefreshPanel(g_hListRight, g_szRightPath);
            }
            break;
        }
            
        case ID_BTN_ARROW_LEFT: {
            // Synchroniser le panneau gauche avec le droit
            if (strlen(g_szRightPath) > 0) {
                lstrcpyA(g_szLeftPath, g_szRightPath);
                RefreshPanel(g_hListLeft, g_szLeftPath);
            }
            break;
        }
            
        case ID_BTN_SETTINGS:
            // Ouvrir la fenêtre de configuration
            ShowSettings();
            break;
            
        case ID_BTN_QUIT:
            // Quitter directement sans confirmation
            PostQuitMessage(0);
            break;
    }
}