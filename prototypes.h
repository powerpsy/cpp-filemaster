#ifndef PROTOTYPES_H
#define PROTOTYPES_H

#include <windows.h>

// =============================================================================
// FILEMASTER - Prototypes de fonctions
// =============================================================================

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

// Fonctions principales de gestion des panneaux
void RefreshPanel(HWND hList, const char* szPath);
BOOL GetRealFileName(const char* szPath, int index, char* szRealName);
void ListDrives(HWND hList);
void GoToParent(int panel);

// Fonctions de commandes (commands.c)
void ExecuteCommand(int cmdId);

// Fonctions d'interface utilisateur
LRESULT CALLBACK ListBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Fonctions de configuration
void ShowSettings();
void LoadSettings();
void SaveSettings();

// Fonctions utilitaires UI
void FlashError(HWND hwnd);
int ShowCenteredMessageBox(const char* text, const char* caption, UINT type);
HWND CreateCenteredWindow(const char* className, const char* windowName, DWORD style, int width, int height, HWND parent, HINSTANCE hInstance);

// Fonctions de visualisation et édition
void ShowASCIIViewer(const char* filename);
void ShowHEXViewer(const char* filename);
void ShowASCIIEditor(const char* filename);
void ShowHEXEditor(const char* filename);
char* FormatASCIIData(const BYTE* data, DWORD size);

// Procédures de fenêtres (Window Procedures)
LRESULT CALLBACK ViewerWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
// Fonctions utilitaires (utils.c)
char* simple_strchr(const char* str, int c);
int simple_atoi(const char* str);
char* simple_strstr(const char* haystack, const char* needle);
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);
int ShowCenteredMessageBox(const char* text, const char* caption, UINT type);
void FlashError(HWND hwnd);
HWND CreateCenteredWindow(const char* className, const char* windowName, DWORD style, int width, int height, HWND parent, HINSTANCE hInstance);

// Fonctions de gestion des paramètres (settings.c)
void LoadSettings();
void SaveSettings();
void ShowSettings();
LRESULT CALLBACK SettingsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Autres callbacks
LRESULT CALLBACK RenameDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MakedirDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ConflictDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SettingsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Fonctions de gestion des conflits (conflict.c)
int ShowConflictDialog(const char* sourceFile, const char* destFile);
void FormatFileInfo(const char* filePath, char* buffer, int bufferSize, const char* label);

// Fonctions de renommage (rename.c) 
BOOL ShowRenameDialog(const char* currentName, char* newName);

// Fonctions de création de dossier (mkdir.c)
BOOL ShowMakedirDialog(char* dirName);

// Fonctions de formatage (utilitaires)
void FormatFileSize(DWORD sizeLow, DWORD sizeHigh, char* buffer);
void FormatHexData(const BYTE* data, DWORD size, char** output);

// Fonctions de gestion de fichiers
BOOL LoadFileData(const char* filename, BYTE** data, DWORD* size);
BOOL SaveFileData(const char* filename, const BYTE* data, DWORD size);

#endif // PROTOTYPES_H