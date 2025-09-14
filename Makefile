# Makefile pour FileMaster
# =============================================================================

# Variables de configuration
TARGET = FileMaster.exe
SOURCE = main.c
MODULES = commands.c interface.c navigation.c rename.c mkdir.c conflict.c viewers.c fileops.c utils.c settings.c
ALL_SOURCES = $(SOURCE) $(MODULES)
RC_FILE = filemaster.rc
RC_OBJ = filemaster.o
LIBS = -luser32 -lkernel32 -lshell32 -lgdi32 -lcomctl32
CFLAGS = -mwindows -Oz -s -fno-unwind-tables -fno-asynchronous-unwind-tables
CC = gcc
WINDRES = windres

# Cibles par défaut
.PHONY: all clean debug release test run

all: release

# Build de release (modulaire par défaut)
release: 
	@echo "=== Build Modulaire ==="
	$(MAKE) SOURCE="$(ALL_SOURCES)" $(TARGET)

# Build de debug (avec symboles)
debug: 
	@echo "=== Build Debug Modulaire ==="
	$(MAKE) TARGET=FileMaster_debug.exe CFLAGS="-mwindows -g -O0" SOURCE="$(ALL_SOURCES)" FileMaster_debug.exe

# Règle principale de compilation
$(TARGET): $(SOURCE) $(RC_OBJ) prototypes.h
	@echo "=== Compilation de $(TARGET) ==="
	$(CC) -o $@ $(SOURCE) $(RC_OBJ) $(LIBS) $(CFLAGS)
	@echo "✅ Compilation terminée"
	@powershell "Write-Host ('Taille: ' + (Get-Item '$@').Length + ' bytes') -ForegroundColor Green"

# Compilation des ressources
$(RC_OBJ): $(RC_FILE) version.h
	@echo "=== Compilation des ressources ==="
	$(WINDRES) $< -o $@

# Tests de compilation
test: $(TARGET)
	@echo "=== Test de l'exécutable ==="
	@powershell "if (Test-Path '$(TARGET)') { Write-Host '✅ $(TARGET) créé avec succès' -ForegroundColor Green } else { Write-Host '❌ Échec de création' -ForegroundColor Red }"

# Lancement de l'application
run: $(TARGET)
	@echo "=== Lancement de $(TARGET) ==="
	./$(TARGET)

# Nettoyage des fichiers générés
clean:
	@echo "=== Nettoyage ==="
	-del /Q $(TARGET) FileMaster_debug.exe $(RC_OBJ) 2>nul
	@echo "✅ Fichiers nettoyés"

# Affichage de l'aide
help:
	@echo "Makefile pour FileMaster"
	@echo ""
	@echo "Cibles disponibles:"
	@echo "  all         - Build de release (défaut, modulaire)"
	@echo "  release     - Build optimisé pour la production"
	@echo "  debug       - Build avec symboles de debug"
	@echo "  test        - Compile et teste la création"
	@echo "  run         - Compile et lance l'application"
	@echo "  clean       - Supprime les fichiers générés"
	@echo "  help        - Affiche cette aide"
	@echo ""
	@echo "Variables:"
	@echo "  TARGET=$(TARGET)"
	@echo "  SOURCE=$(SOURCE)"
	@echo "  MODULES=$(MODULES)"
	@echo "  CFLAGS=$(CFLAGS)"

# Informations sur la version
version:
	@echo "FileMaster Build System"
	@make --version | findstr /C:"GNU Make"
	@gcc --version | findstr /C:"gcc"