# FileMaster - Clone Total Commander

## Interface

L'application présente une interface à 3 panneaux :
- **Panneau gauche** : Liste des fichiers et dossiers du répertoire gauche
- **Panneau central** : Boutons de commandes
- **Panneau droit** : Liste des fichiers et dossiers du répertoire droit
- **Barres de statut** : Affichent les chemins courants des deux panneaux

## Navigation

### Double-clic sur un élément
- **Dossier** (affiché entre crochets `[nom]`) : Entre dans le dossier
- **Lecteur** (affiché comme `[C:]`) : Va à la racine du lecteur
- **Fichier** : Sélectionne le fichier

### Sélection de panneau
- Cliquer sur un panneau le rend actif
- Le panneau actif reçoit les commandes

## Commandes disponibles

### Navigation
- **Ce PC** : Affiche la liste des lecteurs disponibles dans les deux panneaux
- **Parent** : Remonte au répertoire parent du panneau actif
- **Invert** : Change le panneau actif (gauche ↔ droite)
- **Clear** : Vide les deux panneaux

### Opérations sur fichiers (À implémenter)
- **Copy** : Copie le fichier sélectionné vers l'autre panneau
- **Move** : Déplace le fichier sélectionné vers l'autre panneau  
- **Delete** : Supprime le fichier sélectionné (avec confirmation)
- **Rename** : Renomme le fichier sélectionné
- **Makedir** : Crée un nouveau dossier

### Visualisation (À implémenter)
- **Show Asc** : Affiche le contenu du fichier en mode texte
- **Show HEX** : Affiche le contenu du fichier en hexadécimal
- **Edit** : Ouvre l'éditeur de texte pour le fichier
- **Edit Hex** : Ouvre l'éditeur hexadécimal

### Exécution
- **Execute** : Lance le fichier exécutable sélectionné

## Fonctionnalités actuellement implémentées

✅ **Interface graphique** : Panneaux, boutons, barres de statut
✅ **Navigation** : Parcours des dossiers, lecteurs, répertoire parent
✅ **Affichage** : Liste des fichiers avec tailles, distinction dossiers/fichiers
✅ **Sélection** : Panneau actif, sélection d'éléments
✅ **Exécution** : Lancement des fichiers exécutables
✅ **Confirmation** : Dialogue de confirmation pour suppression

## Format d'affichage

- **Dossiers** : `[nom_dossier]`
- **Lecteurs** : `[C:]`, `[D:]`, etc.
- **Fichiers** : `nom_fichier.ext (taille bytes)`

## Taille de l'exécutable

- **Taille actuelle** : ~9KB
- **Dépendances** : KERNEL32.dll, USER32.dll, SHELL32.dll
- **Optimisations** : Compilation avec `-Os -s -nostdlib`

## Prochaines améliorations possibles

1. **Implémentation des opérations de fichiers** : Copy, Move, Delete, Rename, Makedir
2. **Visualiseurs** : Intégrés ASCII et hexadécimal  
3. **Éditeurs** : Texte et hexadécimal simple
4. **Raccourcis clavier** : F3 (View), F4 (Edit), F5 (Copy), F6 (Move), F8 (Delete)
5. **Glisser-déposer** : Support du drag & drop
6. **Tri** : Par nom, taille, date
7. **Filtres** : Affichage sélectif par extension
8. **Historique** : Navigation précédent/suivant
9. **Favoris** : Signets vers dossiers fréquents
10. **Configuration** : Sauvegarde des préférences
