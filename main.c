#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <shellapi.h>
#include "prototypes.h"
#include "constants.h"

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
char g_szSelectedFiles[10][MAX_PATH];
int g_nSelectedCount = 0;

AppSettings g_settings = {0, 0, "C:\\", 
    "Consolas", "Segoe UI", "Segoe UI", "Segoe UI", "Segoe UI",
    12, 9, 12, 12, 12};

// Fonctions de gestion des fichiers pour les viewers/éditeurs


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
    
    g_hMainWindow = CreateWindowA("FM", "FileMaster 2.0", style,
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