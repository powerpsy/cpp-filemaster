# FileMaster 1.0 - Clone Total Commander Ultra-Léger

Clone de Total Commander / File Commander en C ultra-optimisé pour Windows.  
Exécutable de **11KB** avec interface graphique complète et système de préférences.

## Interface

```
┌─────────────────┬─────────────────┬─────────────────┐
│   Panneau       │    Commandes    │   Panneau       │
│   Gauche        │                 │   Droit         │
│                 │   Computer      │                 │
│ Dossier1 DIR    │   Parent        │ Dossier1 DIR    │
│ test.txt 1024 o │   Invert        │ test.txt 1024 o │
│                 │   Clear         │                 │
│                 │   ─────────     │                 │
│                 │   Copy          │                 │
│                 │   Move          │                 │
│                 │   Delete        │                 │
│                 │   Rename        │                 │
│                 │   Makedir       │                 │
│                 │   ─────────     │                 │
│                 │   Show Asc      │                 │
│                 │   Show HEX      │                 │
│                 │   Edit          │                 │
│                 │   Edit Hex      │                 │
│                 │   Execute       │                 │
├─────────────────┼─────────────────┼─────────────────┤
│ C:\Dossier\     │                 │ C:\Dossier\     │
├─────────────────┴─────────────────┴─────────────────┤
│                                    [Settings] [Quit]│
└─────────────────────────────────────────────────────┘
```

## Nouveautés version 1.0

✅ **Affichage tabulaire** : Nom à gauche, taille/type à droite  
✅ **Bouton Settings** : Fenêtre de paramètres avec préférences  
✅ **Bouton Quit** : Sortie avec confirmation  
✅ **Fichier de préférences** : `FileMaster.prefs` sauvegardé automatiquement  
✅ **Formatage des tailles** : bytes/KB/MB selon la taille  

## Fichiers

- `main.c` - Code source principal (~15KB)
- `build.ps1` - Script de build PowerShell automatique
- `filemaster.exe` - Exécutable Windows (11264 octets)
- `FileMaster.prefs` - Préférences utilisateur (créé automatiquement)
- `FONCTIONNALITES.md` - Documentation détaillée
- `CHANGELOG.md` - Nouveautés version 1.0

## Compilation

### Automatique (recommandé)
```powershell
.\build.ps1           # Compile et lance
.\build.ps1 build     # Compile seulement
.\build.ps1 run       # Lance l'application
.\build.ps1 analyze   # Analyse détaillée
.\build.ps1 clean     # Nettoie
```

### Manuelle
```bash
gcc -o filemaster.exe main.c -luser32 -lkernel32 -lshell32 -Os -s -nostdlib -mwindows
```

## Caractéristiques

- **Taille** : 11264 octets (~11KB)
- **Dépendances** : 3 DLL Windows (KERNEL32.dll, USER32.dll, SHELL32.dll)
- **Interface** : Double panneau + 14 boutons + 2 boutons système
- **Préférences** : Sauvegarde automatique des paramètres
- **Type** : Application GUI Windows native
- **Langage** : C pur (pas de runtime C++)

## Fonctionnalités implémentées

✅ **Navigation** : Parcours dossiers, lecteurs, répertoire parent  
✅ **Affichage** : Dual-panel avec format tabulaire et barres de statut  
✅ **Sélection** : Panneau actif, sélection d'éléments  
✅ **Exécution** : Lancement des fichiers exécutables  
✅ **Commandes** : 14 boutons opérationnels  
✅ **Système** : Settings, Quit avec confirmation  
✅ **Préférences** : Fichiers cachés, tri, répertoire par défaut  

## Optimisations appliquées

- `-Os` : Optimisation pour la taille
- `-s` : Suppression des symboles de debug
- `-nostdlib` : Pas de runtime C standard
- `-mwindows` : Application GUI (pas de console)
- Code C minimal avec API Windows directe

**Résultat : L'application Windows native la plus légère possible en C !**
