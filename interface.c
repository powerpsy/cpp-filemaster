// interface.c - Module d'interface utilisateur et gestion des contrôles
// ====================================================================

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "prototypes.h"
#include "constants.h"

// Variables externes
extern HWND g_hListLeft, g_hListRight;
extern HWND g_hMainWindow, g_hStatusLeft, g_hStatusRight;
extern HWND g_hArrowRight, g_hArrowLeft;
extern char g_szLeftPath[MAX_PATH], g_szRightPath[MAX_PATH];
extern int g_nActivePanel;
extern WNDPROC g_oldListProcLeft, g_oldListProcRight;
extern int g_lastClickedItem;
extern HFONT g_hFontUTF8, g_hFontMono;
extern HBRUSH g_hBrushActive, g_hBrushInactive;

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

// Procédure principale de fenêtre
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
                "Makedir", "Show ASC", "Show HEX", "Edit ASC",
                "Edit HEX", "Execute"
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