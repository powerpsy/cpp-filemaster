# Changelog

Toutes les modifications notables de ce projet seront documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0.0] - 2025-09-14

### ADDED - ARCHITECTURE MODULAIRE COMPLÈTE
- **Extraction complète en modules** : Refactorisation totale de l'architecture monolithique
- **Module interface.c** : Gestion complète de l'interface utilisateur et procédures de fenêtres
- **Module commands.c** : Logique métier centralisée pour toutes les commandes
- **Module navigation.c** : Fonctions de navigation et gestion des panneaux
- **Module constants.h** : Centralisation de tous les identifiants de contrôles
- **Intégration viewers/éditeurs** : Show ASC/HEX et Edit ASC/HEX pleinement fonctionnels

### IMPROVED
- **Interface cohérente** : Libellés uniformes "Show ASC/HEX" et "Edit ASC/HEX"
- **Validation robuste** : Vérifications complètes des fichiers sélectionnés
- **Messages d'erreur** : Feedback utilisateur approprié pour chaque opération
- **Gestion des conflits** : Amélioration des opérations de copie/déplacement

### TECHNICAL
- **Modularité avancée** : 11 modules spécialisés avec séparation claire des responsabilités
- **Code maintenable** : Architecture claire pour extensions futures
- **Performance maintenue** : Taille optimisée ~48KB malgré les fonctionnalités étendues
- **Compilation robuste** : Build system mis à jour pour l'architecture modulaire

---

## [1.0.0.0] - 2025-09-14

### ADDED
- Interface double panneau pour navigation de fichiers
- Opérations de base : copie, déplacement, suppression, renommage
- Création de dossiers et navigation parente
- Visualiseurs texte ASCII et hexadécimal
- Éditeur de fichiers texte intégré
- Exécution de programmes
- Système de recherche dans les fichiers
- Sélection multiple et opérations par lot
- Configuration des préférences utilisateur
- Support complet Windows avec métadonnées de version
- Compilation optimisée (48 KB)

### FIXED
- Centrage des boîtes de dialogue sur l'application parente
- Alignement correct des tailles de fichiers (police monospace)
- Boutons de la fenêtre settings centrés
- Confirmation de fermeture d'application
- Gestion correcte des événements clavier et souris

### TECHNICAL
- Architecture modulaire avec core/pool/commands
- Ressources de version Windows intégrées
- Support DPI-aware pour écrans haute résolution
- Compatibilité Windows 7/8/8.1/10/11
- Build automatisé avec scripts PowerShell

---

## Versions précédentes (historique de développement)

# FileMaster 1.0 - bugs et améliorations

x1. quand une fenêtre s'ouvre sur l'applciation (uibox, msgbox, toutes les input box, etc), elle doit être centrée sur la position l'application. ceci est valable pour toutes les fenetres de l'application, y compris les fenêtres de confirmation quand on quitte l'applciation

x2. Appuyer sur le bouton "Quit" et l'appui sur la croix pour fermer l'application quitte l'application, pas de confirmation.

x3. sur les panneaux de droite et gauche, les fichiers sont énumérés et la taille des fichiers (ou l'annotation "DIR") ne sont pas alignés à droite. Cela peut provenir de la sélection de la police de caractère. on peut utiliser une police monospaced

x4. la fenêtre de "settings" a 2 choix Ok et Annuler. Il faut centrer les boutons dans la fenêtre en bas. ensuite il faut appliquer les choix si on appuie sur OK. Annuler ferme la fenetre de settings.

x5. Il faut utiliser une police de caractère qui gère les accents (par ex sur la barre de titre des settings ou dans la fenêtre des settings, il y a des caractères avec accents mal affichés)

x6. le fond de l a fenêtre de répertoire actif est RGB 102-136-187, le fond de du répertoire inactif est 170-170-170

x7. La commande "Ce PC" doit être renommée en "Home". Cette fonction liste les différents disques de l'ordinateur sur la fenêtre active. La commande actuelle feme l'application au lieu de lister les lecteurs.

x8. Bug: les actions de la fenetre settings ne sont pas appliquées quand on clique sur OK et Annuler: Il faut enregistrer les options dans FileMaster.prefs quand OK est pressé, appliquer les paramètres et fermer la fenêtre de settings. Quand on clique sur Annuler, il suffit de fermer la fenêtre de settings sans changer les paramètres.

x9. Lorsque la fenetre de settings est ouverte, on peut encore interagir avec la fenetre principale de l'application derriere la fenetre de settings. J'aimerais qu'on ne puisse plus interagir sur quoi que ce soit (à part les settings) jusqu'à ce que la fenetre de settings soit fermée.

x10. quand on clique dans le panneau de gauche ou de droite, il devient actif. ce n'es pas uniquement lirsqu'on clique sur un fichier.

x11. fonctionnement de la commande "invert": sélectionne tous les fichiers du répertoire courant sauf ceux qui sont déjà sélectionnées.

x12. il y a 44 caractères max à afficher pour le nom du fichier + la taille dans les panneaux 1 & 2. les 2 derniers caractères contiennent l'unité de taille (o, ko, Mo, Go) il y a un espace et la taille au format x'xxx (5 caractères) puis un espace. Il reste 44-2-1-5-1=35 caractères pour le nom au max. Si le nom dépasse 35 caractères, il faut couper et mettre le caractère "..." avant l'espace et la taille du fichier

x13. le format de la taille est le suivant: il faut toujours afficher 5 caractères, x'xxx ou xxx.x ou xx.xx ou x.xxx

x14. fonctionnement de la commande "clear": sélectionne rien dans la fenêtre courante.

x15. fonctionnement de la commande "copy": copie les fichiers sélectionnées depuis la fenêtre active (source) vers le panneau non actif (destination). avant de faire la copie, il faut gérer les conflits; une tick box dans la poupup de gestion de conflit doit permettre d'appliquer ce choix (écraser, passer, annuler) à tous les fichiers suivants.

x16. bug: quand on navigue vers un répertoire avec un espace, le répertoire s'ouvre mais sans l'espace et la suite du nom. par ex: "RETROID 3" --> on ouvre le répertoire "RETROID" qui aparait vide

x17. quand on sélectionne des fichiers troncaturés  pour l'affichage avec "..." à la fin, on ne peut pas les copier. il faut reprendre le nom réel du fichier pour le copier

x18. on va changer le mode de sélection des fichiers: comme sous windows de base (quand on clique sur un fichier cela supprime la sélection en cours et sélectionne le fichier sous la souris. pour sélectionner plusieurs fichier, il faut maintenir CRTL et cliquer; pour une range sélection, il faut cliquer sur un chichier et avec SHIFT d'appuyé, il faut cliquer sur le dernier fichier à sélectionner, tout ce qui est entre est sélectionné)

x19. en cas de conflit dans la fonction "copy", il faut afficher les détails des deux fichiers sur 2 lignes dans la fenetre de dialogue: source: le nom, la taille, la date
destination: le nom, la taille, la date.

x20. fonctionnement de la commande "move": identique à la fonction "copy" mais en plus, cela efface le fichier d'origine si il a été copié vers la destination (sinon, il faut le garder)

x21. fonctionnement de la commande "delete": supprime les fichiers sélectionnés

x22. fonctionnement de la commande "rename": renomme un fichier.

x23. amélioration UX: suppression de toutes les fenêtres de résumé après les actions copy, move et delete pour fluidifier l'utilisation

x24. fonctionnement de la commande "makedir": crée un répertoire dans la fenetre active

x25. Amélioration: à droite de la barre de status du panneau de gauche et à gauche de la barre de status du panneau de droite tu peux mettre un bouton avec une fléche qui va en direction de l'autre panneau. Si on clique sur la petite flèche, le répertoire en cours du panneau où la fleche est cliqué devient le répertoire en cours de l'autre panneau

x26. fonctionnement de la commande "Show ASC": affiche le premier fichier sélectionné dans un viewer fait maison. cela affiche les caractères en ASCII du fichier dans une fenêtre qui s'ouvre avec exactement la même taille que l'interface de l'application.

x27. fonctionnement de la commande "Show HEX": affiche le premier fichier sélectionné dans un viewer fait maison. cela affiche les caractères en HEX sur 4 colonnes de 4 chiffres en HEX (FFFFFFFF), la cinquième colonne affiche en ASCII les 16 caractères ASCII des 4 premières colonnes.

x28. fonctionnement des commandes "Edit" et "Edit HEX". cela fait la même chose que les actions "show" mais l'utilisateur peut modifier le fichier et il y a un bouton enregistrer sur la fenetre. Si la fenêtre est fermée avec modification, il y a une popup qui propose de sauvegarder cae il y a des changements.

x29. Il faut mettre en place la sauvegarde du fichier de l'action "Edit ASC" et "Edit HEX" s'il y a une modification.

x30. Lorsqu'on démarre l'application, la fenêtre s'ouvre au centre du desktop.

x31. L'éditeur et le viewer HEX affichent sur 8 colonnes de 4 chiffres en HEX (FFFFFFFF), la cinquième colonne affiche en ASCII les 32 caractères ASCII des 8 colonnes en HEX.

32. sur le viewer et l'éditeur HEX, est ce que tu peux changer le curseur pour un rectangle plein au lieu de la barre verticale fine (comme lorsqu'on est en mode "insert"). le curseur (et les sélections) sont répliquées sur l'autre côté (par exemple, on se déplace sur le fichier côté HEX, le deuxième curseur se déplace sur l'équivalent côté ASCII. et inversement) --> à garder pour la version 2.0.

x33. tu peux ajouter un bouton "Fermer" au centre des viewers et des éditeurs ASCII et HEX pour fermer la fenêtre en cours.

x34. Dans le viewer et l'éditeur ASCII et HEX il y a une boite de dialogue qui s'ouvre en disant que c'est "impossible de charger le fichier"  j'ai l'impression que cela arrive quand le nom de fichier est long et est affiché "..." à la fin dans le panneau. tu peux identifier le probléème et corriger ?

x35. le viewer et l'éditeur ASCII n'affichent pas tous les caractères des fichiers. parfois il y a juste l'en-tête du fichier affiché et puis plus rien.