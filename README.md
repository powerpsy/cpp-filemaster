# FileMaster - Gestionnaire de Fichiers Modulaire

Clone de Total Commander / File Commander en C avec architecture modulaire optimisée pour Windows.  
Exécutable de **48KB** avec interface graphique complète et organisation modulaire.

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

## 🏗️ Architecture Modulaire

Le projet utilise une architecture modulaire propre :

```
├── main.c          # Core application (1528 lignes)
├── rename.c        # Fonctions de renommage (126 lignes)
├── mkdir.c         # Création de dossiers (87 lignes)
├── conflict.c      # Résolution de conflits (148 lignes)
├── viewers.c       # Visualiseurs/éditeurs (343 lignes)
├── fileops.c       # Opérations fichiers (151 lignes)
├── prototypes.h    # Déclarations centralisées (40+ fonctions)
└── Makefile        # Système de build modulaire
```

### Modules Fonctionnels

✅ **`rename.c`** - Gestion des renommages de fichiers/dossiers  
✅ **`mkdir.c`** - Interface de création de nouveaux dossiers  
✅ **`conflict.c`** - Résolution des conflits lors d'opérations  
✅ **`viewers.c`** - Visualiseurs ASCII/HEX et éditeurs  
✅ **`fileops.c`** - Opérations de base sur les fichiers  

## 🛠️ Compilation

### Build Modulaire (par défaut)
```bash
mingw32-make              # Build de release modulaire
mingw32-make debug        # Build de debug modulaire
mingw32-make clean        # Nettoyer
mingw32-make help         # Aide
```

### Optimisations
- **Taille finale** : 48KB (architecture modulaire)
- **Flags** : `-Oz -s -fno-unwind-tables`
- **Modules** : Compilation séparée et linkage optimisé

## 📋 Fonctionnalités

### Navigation & Interface
- **Double panneau** avec navigation indépendante
- **Sélection multiple** avec Ctrl+clic
- **Formatage intelligent** des tailles (bytes/KB/MB)
- **Gestion des conflits** avec options "Appliquer à tous"

### Raccourcis Clavier
- **F5** - Copier | **F6** - Déplacer | **F7** - Nouveau dossier | **F8** - Supprimer
- **F3** - Visualiseur ASCII | **F4** - Éditeur ASCII | **F9** - Renommer
- **Shift+F3** - Visualiseur HEX | **Shift+F4** - Éditeur HEX

### Modules Intégrés
- **Visualiseurs** ASCII/HEX avec scroll et recherche
- **Éditeurs** ASCII/HEX avec sauvegarde
- **Gestionnaire de conflits** intelligent
- **Système de préférences** persistant

## 📦 Fichiers du Projet

```
FileMaster/
├── main.c              # Core application (1528 lignes)
├── rename.c            # Module renommage (126 lignes)
├── mkdir.c             # Module création dossiers (87 lignes)
├── conflict.c          # Module résolution conflits (148 lignes)
├── viewers.c           # Module visualiseurs/éditeurs (343 lignes)
├── fileops.c           # Module opérations fichiers (151 lignes)
├── prototypes.h        # Déclarations centralisées
├── Makefile            # Système de build modulaire
├── filemaster.rc       # Ressources Windows
├── filemaster.manifest # Manifeste d'application
├── version.h           # Informations de version
└── FileMaster.exe      # Exécutable final (48KB)
```

## 🏆 Performances

- **Taille** : 48KB (architecture modulaire optimisée)
- **Startup** : Instantané (<100ms)
- **Mémoire** : Empreinte minimale
- **Compilation** : <2 secondes pour build complet

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
