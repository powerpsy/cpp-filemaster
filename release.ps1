# release.ps1 - Script de release automatisé pour FileMaster
# Usage: .\release.ps1 [major|minor|patch|build] ["Description du changement"]

param(
    [ValidateSet("major", "minor", "patch", "build")]
    [string]$VersionType = "build",
    [string]$Description = "Version release"
)

# Configuration
$VERSION_FILE = "version.h"
$CHANGELOG_FILE = "CHANGELOG.md"
$EXE_NAME = "FileMaster.exe"
$REPO_NAME = "cpp-filemaster"

Write-Host "=== FILEMASTER RELEASE AUTOMATION ===" -ForegroundColor Cyan
Write-Host "Type de version: $VersionType" -ForegroundColor Yellow
Write-Host "Description: $Description" -ForegroundColor Yellow

# 1. Lecture de la version actuelle
function Get-CurrentVersion {
    $content = Get-Content $VERSION_FILE
    $major = ($content | Select-String "#define VERSION_MAJOR\s+(\d+)").Matches[0].Groups[1].Value
    $minor = ($content | Select-String "#define VERSION_MINOR\s+(\d+)").Matches[0].Groups[1].Value
    $patch = ($content | Select-String "#define VERSION_PATCH\s+(\d+)").Matches[0].Groups[1].Value
    $build = ($content | Select-String "#define VERSION_BUILD\s+(\d+)").Matches[0].Groups[1].Value
    
    return @{
        Major = [int]$major
        Minor = [int]$minor  
        Patch = [int]$patch
        Build = [int]$build
    }
}

# 2. Calcul de la nouvelle version
function Get-NewVersion {
    param($current, $type)
    
    $new = $current.Clone()
    
    switch ($type) {
        "major" { 
            $new.Major++; $new.Minor = 0; $new.Patch = 0; $new.Build = 0 
        }
        "minor" { 
            $new.Minor++; $new.Patch = 0; $new.Build = 0 
        }
        "patch" { 
            $new.Patch++; $new.Build = 0 
        }
        "build" { 
            $new.Build++ 
        }
    }
    
    return $new
}

# 3. Mise à jour du fichier version.h
function Update-VersionFile {
    param($version)
    
    $content = Get-Content $VERSION_FILE
    $content = $content -replace "#define VERSION_MAJOR\s+\d+", "#define VERSION_MAJOR    $($version.Major)"
    $content = $content -replace "#define VERSION_MINOR\s+\d+", "#define VERSION_MINOR    $($version.Minor)"
    $content = $content -replace "#define VERSION_PATCH\s+\d+", "#define VERSION_PATCH    $($version.Patch)"
    $content = $content -replace "#define VERSION_BUILD\s+\d+", "#define VERSION_BUILD    $($version.Build)"
    
    Set-Content $VERSION_FILE $content
    Write-Host "✅ version.h mis à jour" -ForegroundColor Green
}

# 4. Mise à jour du CHANGELOG
function Update-Changelog {
    param($version, $description)
    
    $versionString = "$($version.Major).$($version.Minor).$($version.Patch).$($version.Build)"
    $date = Get-Date -Format "yyyy-MM-dd"
    
    $newEntry = @"
## [$versionString] - $date

### $($VersionType.ToUpper())
- $description

"@
    
    $content = Get-Content $CHANGELOG_FILE -Raw
    $insertPosition = $content.IndexOf("## [")
    
    if ($insertPosition -eq -1) {
        # Premier changelog
        $newContent = "# Changelog`n`n$newEntry`n$content"
    } else {
        # Insertion avant la première version existante
        $newContent = $content.Insert($insertPosition, "$newEntry`n")
    }
    
    Set-Content $CHANGELOG_FILE $newContent
    Write-Host "✅ CHANGELOG.md mis à jour" -ForegroundColor Green
}

# 5. Compilation avec Make
function Build-Application {
    Write-Host "🔨 Compilation avec Make..." -ForegroundColor Yellow
    
    # Nettoyage d'abord
    & make clean
    if ($LASTEXITCODE -ne 0) {
        Write-Host "⚠️ Avertissement lors du nettoyage" -ForegroundColor Yellow
    }
    
    # Compilation avec Make
    & make release
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Erreur compilation avec Make" -ForegroundColor Red
        exit 1
    }
    
    # Vérification et affichage de la taille
    if (Test-Path $EXE_NAME) {
        $size = (Get-Item $EXE_NAME).Length
        Write-Host "✅ Compilation réussie avec Make - Taille: $([math]::Round($size/1KB, 1)) KB" -ForegroundColor Green
    } else {
        Write-Host "❌ Exécutable non trouvé après compilation" -ForegroundColor Red
        exit 1
    }
}

# 6. Commit et tag Git
function Commit-And-Tag {
    param($version, $description)
    
    $versionString = "$($version.Major).$($version.Minor).$($version.Patch).$($version.Build)"
    $tagName = "v$versionString"
    
    # Ajout des fichiers modifiés
    & git add $VERSION_FILE $CHANGELOG_FILE $EXE_NAME
    
    # Commit
    $commitMessage = "Release $versionString - $description"
    & git commit -m $commitMessage
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Commit créé: $commitMessage" -ForegroundColor Green
        
        # Création du tag
        & git tag -a $tagName -m "Version $versionString`n`n$description"
        Write-Host "✅ Tag créé: $tagName" -ForegroundColor Green
        
        # Push (optionnel - décommentez si souhaité)
        # & git push origin main --tags
        # Write-Host "✅ Poussé vers GitHub" -ForegroundColor Green
    } else {
        Write-Host "❌ Erreur lors du commit" -ForegroundColor Red
    }
}

# EXECUTION PRINCIPALE
$currentVersion = Get-CurrentVersion
$newVersion = Get-NewVersion $currentVersion $VersionType

$currentStr = "$($currentVersion.Major).$($currentVersion.Minor).$($currentVersion.Patch).$($currentVersion.Build)"
$newStr = "$($newVersion.Major).$($newVersion.Minor).$($newVersion.Patch).$($newVersion.Build)"

Write-Host "Version actuelle: $currentStr" -ForegroundColor White
Write-Host "Nouvelle version: $newStr" -ForegroundColor Green

# Confirmation
$confirm = Read-Host "Continuer? (y/N)"
if ($confirm -ne "y" -and $confirm -ne "Y") {
    Write-Host "Annulé." -ForegroundColor Yellow
    exit 0
}

# Exécution des étapes
Update-VersionFile $newVersion
Update-Changelog $newVersion $Description
Build-Application
Commit-And-Tag $newVersion $Description

Write-Host "`n🎉 RELEASE $newStr TERMINÉE AVEC SUCCÈS!" -ForegroundColor Green
Write-Host "Prochaines étapes:" -ForegroundColor Cyan
Write-Host "  git push origin main --tags  # Pour pousser vers GitHub" -ForegroundColor Gray