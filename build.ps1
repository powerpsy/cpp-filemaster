# build.ps1 - Script de build pour FileMaster ultra-léger

param(
    [string]$Action = "all"
)

$TARGET = "FileMaster.exe"
$SOURCE = "main.c"
$LIBS = "-luser32 -lkernel32 -lshell32 -lgdi32"
$CFLAGS = "-Os -s -nostdlib -mwindows"
$SIZE_LIMIT = 4096

function Build {
    Write-Host "=== Compilation optimisée ===" -ForegroundColor Yellow
    
    # Tentative de suppression avec retry
    if (Test-Path $TARGET) { 
        try {
            Remove-Item $TARGET -Force
            Start-Sleep -Milliseconds 100
        } catch {
            Write-Host "ATTENTION: Impossible de supprimer $TARGET (probablement en cours d'exécution)" -ForegroundColor Yellow
            $TARGET = "FileMaster_new.exe"
        }
    }
    
    & gcc -o $TARGET $SOURCE $LIBS.Split() $CFLAGS.Split()
    
    if (Test-Path $TARGET) {
        Write-Host "=== Taille de l'exécutable ===" -ForegroundColor Green
        Get-Item $TARGET | Select-Object Name, Length | Format-Table
        
        Write-Host "=== Analyse des imports ===" -ForegroundColor Green
        & objdump -p $TARGET | Select-String "DLL Name:"
        
        Write-Host "=== Compilation terminée ===" -ForegroundColor Green
        return $true
    } else {
        Write-Host "ERREUR: Compilation échouée" -ForegroundColor Red
        return $false
    }
}

function Run {
    if (Test-Path $TARGET) {
        Write-Host "=== Lancement de l'application ===" -ForegroundColor Cyan
        & ".\$TARGET"
    } else {
        Write-Host "ERREUR: $TARGET n'existe pas. Lancez 'build' d'abord." -ForegroundColor Red
    }
}

function Clean {
    if (Test-Path $TARGET) { 
        Remove-Item $TARGET -Force 
        Write-Host "Répertoire nettoyé" -ForegroundColor Green
    }
}

function Analyze {
    if (Build) {
        Write-Host "=== Analyse complète de l'exécutable ===" -ForegroundColor Magenta
        & objdump -h $TARGET
        Write-Host ""
        & objdump -p $TARGET | Select-String "Member-Name"
    }
}

function Test-Size {
    if (Build) {
        $size = (Get-Item $TARGET).Length
        if ($size -le $SIZE_LIMIT) {
            Write-Host "SUCCESS: Exécutable de $size octets <= 4Ko" -ForegroundColor Green
        } else {
            Write-Host "WARNING: Exécutable de $size octets > 4Ko" -ForegroundColor Red
        }
    }
}

function Show-Help {
    Write-Host "Commandes disponibles:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1           - Compile et lance l'application"
    Write-Host "  .\build.ps1 build     - Compile seulement"
    Write-Host "  .\build.ps1 run       - Lance l'application"
    Write-Host "  .\build.ps1 clean     - Nettoie les fichiers"
    Write-Host "  .\build.ps1 analyze   - Analyse détaillée"
    Write-Host "  .\build.ps1 test-size - Vérifie que la taille <= 4Ko"
    Write-Host "  .\build.ps1 help      - Affiche cette aide"
}

# Dispatcher
switch ($Action.ToLower()) {
    "all"      { if (Build) { Run } }
    "build"    { Build }
    "run"      { Run }
    "clean"    { Clean }
    "analyze"  { Analyze }
    "test-size"{ Test-Size }
    "help"     { Show-Help }
    default    { Show-Help }
}
