// fileops.c - Opérations sur les fichiers et formatage
#include <windows.h>
#include <stdio.h>
#include "prototypes.h"

// Formate la taille d'un fichier en unités lisibles
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

// Charge les données d'un fichier en mémoire
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

// Formate les données ASCII en remplaçant les caractères non-imprimables
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

// Sauvegarde les données dans un fichier
BOOL SaveFileData(const char* filename, const BYTE* data, DWORD size) {
    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    
    DWORD bytesWritten;
    BOOL result = WriteFile(hFile, data, size, &bytesWritten, NULL) && bytesWritten == size;
    CloseHandle(hFile);
    return result;
}

// Formate les données en affichage hexadécimal
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