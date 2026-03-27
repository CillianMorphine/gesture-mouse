# =============================================================
# docs\scripts\backup.ps1 — Automated backup for Windows
#
# Usage:
#   .\backup.ps1              # full backup
#   .\backup.ps1 -Incremental # incremental
#   .\backup.ps1 -Verify      # verify latest backup
#   .\backup.ps1 -List        # list backups
#   .\backup.ps1 -Cleanup     # remove old backups
# =============================================================
param(
    [switch]$Incremental,
    [switch]$Verify,
    [switch]$List,
    [switch]$Cleanup,
    [int]$KeepDays = 7
)

$BackupRoot   = "$env:USERPROFILE\.gesture-mouse-backups"
$ConfigDir    = "$env:APPDATA\GestureMouse"
$InstallDir   = "C:\Program Files\GestureMouse"
$Timestamp    = Get-Date -Format "yyyyMMdd_HHmmss"

New-Item -ItemType Directory -Path $BackupRoot -Force | Out-Null

function Write-Log($msg, $color="Green") {
    $time = Get-Date -Format "HH:mm:ss"
    Write-Host "[$time] $msg" -ForegroundColor $color
}

function Do-FullBackup {
    $BackupName = "backup-full-$Timestamp"
    $BackupDir  = "$BackupRoot\$BackupName"
    $Archive    = "$BackupRoot\$BackupName.zip"

    Write-Log "Starting FULL backup..."
    New-Item -ItemType Directory -Path $BackupDir -Force | Out-Null

    # Config
    if (Test-Path $ConfigDir) {
        Copy-Item -Recurse $ConfigDir "$BackupDir\user-config" -Force
        Write-Log "Config backed up: $ConfigDir"
    }

    # Binary
    $Binary = "$InstallDir\gesture_mouse.exe"
    if (Test-Path $Binary) {
        Copy-Item $Binary "$BackupDir\gesture_mouse.exe" -Force
        Write-Log "Binary backed up"
    }

    # Logs
    New-Item -ItemType Directory -Path "$BackupDir\logs" -Force | Out-Null
    Get-ChildItem -Path "." -Filter "gesture_mouse*.log" | ForEach-Object {
        Copy-Item $_.FullName "$BackupDir\logs\" -Force
    }
    Write-Log "Logs backed up"

    # Metadata
    @"
Backup type: full
Created: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss") UTC
Hostname: $env:COMPUTERNAME
User: $env:USERNAME
"@ | Set-Content "$BackupDir\backup-info.txt"

    # Create archive
    Compress-Archive -Path $BackupDir -DestinationPath $Archive -Force
    Remove-Item -Recurse -Force $BackupDir

    # Checksum
    $hash = (Get-FileHash $Archive -Algorithm SHA256).Hash
    "$hash  $(Split-Path $Archive -Leaf)" | Set-Content "$Archive.sha256"

    $size = [math]::Round((Get-Item $Archive).Length / 1MB, 2)
    Write-Log "Full backup complete: $Archive (${size} MB)"
    return $Archive
}

function Do-Verify {
    $Latest = Get-ChildItem "$BackupRoot\backup-*.zip" |
              Sort-Object LastWriteTime -Descending | Select-Object -First 1

    if (-not $Latest) { Write-Log "No backups found" Red; return }

    Write-Log "Verifying: $($Latest.Name)"

    # Test ZIP integrity
    try {
        $null = [System.IO.Compression.ZipFile]::OpenRead($Latest.FullName)
        Write-Log "Archive integrity: OK"
    } catch {
        Write-Log "Archive integrity: FAILED — $($_.Exception.Message)" Red
        return
    }

    # Checksum
    $sha256File = "$($Latest.FullName).sha256"
    if (Test-Path $sha256File) {
        $expectedHash = (Get-Content $sha256File).Split(" ")[0].ToUpper()
        $actualHash   = (Get-FileHash $Latest.FullName -Algorithm SHA256).Hash
        if ($expectedHash -eq $actualHash) {
            Write-Log "Checksum: OK"
        } else {
            Write-Log "Checksum MISMATCH!" Red
        }
    } else {
        Write-Log "No checksum file found" Yellow
    }

    Write-Log "Verification complete"
}

function Do-List {
    Write-Host "`nBackups in $BackupRoot`n" -ForegroundColor Cyan
    $backups = Get-ChildItem "$BackupRoot\backup-*.zip" | Sort-Object LastWriteTime -Descending
    if (-not $backups) { Write-Host "  No backups found."; return }
    foreach ($b in $backups) {
        $size = [math]::Round($b.Length / 1MB, 2)
        Write-Host "  $($b.LastWriteTime.ToString('yyyy-MM-dd'))  ${size} MB  $($b.Name)"
    }
    Write-Host "`nTotal: $($backups.Count) backup(s)"
}

function Do-Cleanup {
    Write-Log "Cleaning up backups older than $KeepDays days..."
    $cutoff = (Get-Date).AddDays(-$KeepDays)
    $removed = 0
    Get-ChildItem "$BackupRoot\backup-*.zip" | Where-Object { $_.LastWriteTime -lt $cutoff } | ForEach-Object {
        Remove-Item $_.FullName -Force
        Remove-Item "$($_.FullName).sha256" -Force -ErrorAction SilentlyContinue
        $removed++
    }
    Write-Log "Removed $removed old backup(s)"
}

# Dispatch
if     ($Verify)      { Do-Verify }
elseif ($List)        { Do-List }
elseif ($Cleanup)     { Do-Cleanup }
elseif ($Incremental) { Write-Log "Incremental backup not yet implemented — running full backup" Yellow; Do-FullBackup }
else                  { Do-FullBackup }
