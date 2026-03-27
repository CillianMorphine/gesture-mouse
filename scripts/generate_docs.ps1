# =============================================================
# scripts/generate_docs.ps1 — Generate Doxygen docs on Windows
# Usage:
#   .\scripts\generate_docs.ps1         # generate
#   .\scripts\generate_docs.ps1 -Open   # generate + open browser
# =============================================================
param([switch]$Open, [switch]$Check)

Write-Host "`n╔══════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host  "║   GestureMouse — Generate Docs       ║" -ForegroundColor Cyan
Write-Host  "╚══════════════════════════════════════╝`n" -ForegroundColor Cyan

# ── Check Doxygen ─────────────────────────────────────────────
$doxygen = Get-Command doxygen -ErrorAction SilentlyContinue
if (-not $doxygen) {
    Write-Host "✖ doxygen not found!" -ForegroundColor Red
    Write-Host "  Install: winget install doxygen.doxygen" -ForegroundColor Yellow
    exit 1
}
$ver = (doxygen --version 2>&1)[0]
Write-Host "✔ doxygen $ver" -ForegroundColor Green

$dot = Get-Command dot -ErrorAction SilentlyContinue
if ($dot) {
    Write-Host "✔ Graphviz found — diagrams enabled" -ForegroundColor Green
} else {
    Write-Host "⚠ Graphviz not found. Install: winget install Graphviz.Graphviz" -ForegroundColor Yellow
}

if ($Check) {
    $out = doxygen Doxyfile 2>&1
    $w = ($out | Select-String "warning:").Count
    Write-Host "Warnings: $w" -ForegroundColor $(if ($w -eq 0) { "Green" } else { "Yellow" })
    exit 0
}

# ── Generate ──────────────────────────────────────────────────
Write-Host "`nGenerating documentation..." -ForegroundColor White
New-Item -ItemType Directory -Path "docs\generated" -Force | Out-Null

$output = doxygen Doxyfile 2>&1
$warnings = ($output | Select-String "warning:").Count
$errors   = ($output | Select-String "error:").Count

$output | Select-String "(warning|error):" | Select-Object -First 20 | ForEach-Object {
    Write-Host "  $_" -ForegroundColor Yellow
}

Write-Host "`nResults:" -ForegroundColor White
Write-Host "  Warnings : $warnings" -ForegroundColor $(if ($warnings -eq 0) { "Green" } else { "Yellow" })
Write-Host "  Errors   : $errors"   -ForegroundColor $(if ($errors   -eq 0) { "Green" } else { "Red" })
Write-Host "  Output   : docs\generated\html\index.html" -ForegroundColor Green

Write-Host "`n✔ Documentation generated!" -ForegroundColor Green

if ($Open) {
    $index = "docs\generated\html\index.html"
    if (Test-Path $index) {
        Start-Process $index
    }
}
