# =============================================================
# scripts/lint.ps1 — Static analysis runner for Windows
# Usage:
#   .\scripts\lint.ps1           # check only
#   .\scripts\lint.ps1 -Fix      # apply auto-fixes
#   .\scripts\lint.ps1 -Format   # format with clang-format
# =============================================================
param(
    [switch]$Fix,
    [switch]$Format
)

$ErrorCount = 0
$BuildDir   = "build"

Write-Host "`n╔══════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host  "║   GestureMouse — Static Analysis     ║" -ForegroundColor Cyan
Write-Host  "╚══════════════════════════════════════╝`n" -ForegroundColor Cyan

# ── Step 1: clang-format ─────────────────────────────────────
Write-Host "[1/3] clang-format" -ForegroundColor White

$cfmt = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $cfmt) {
    Write-Host "  ⚠  clang-format not found — install LLVM from https://llvm.org/" -ForegroundColor Yellow
} else {
    $files = Get-ChildItem -Recurse -Include "*.cpp","*.h" src,include -ErrorAction SilentlyContinue
    $changed = 0
    foreach ($file in $files) {
        if ($Format) {
            clang-format -i $file.FullName
            Write-Host "  ✔ formatted: $($file.Name)" -ForegroundColor Green
        } else {
            $result = clang-format --dry-run --Werror $file.FullName 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Host "  ⚠ needs formatting: $($file.Name)" -ForegroundColor Yellow
                $changed++
            }
        }
    }
    if ($changed -eq 0 -and -not $Format) {
        Write-Host "  ✔ All files correctly formatted" -ForegroundColor Green
    }
    $ErrorCount += $changed
}

# ── Step 2: clang-tidy ───────────────────────────────────────
Write-Host "`n[2/3] clang-tidy" -ForegroundColor White

$ctidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $ctidy) {
    Write-Host "  ⚠  clang-tidy not found" -ForegroundColor Yellow
    Write-Host "     Install LLVM: https://github.com/llvm/llvm-project/releases" -ForegroundColor Gray
} else {
    $compileCommands = "$BuildDir\compile_commands.json"
    if (-not (Test-Path $compileCommands)) {
        Write-Host "  ⚠  compile_commands.json not found." -ForegroundColor Yellow
        Write-Host "     Run: cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B $BuildDir" -ForegroundColor Gray
    } else {
        $fixFlag = if ($Fix) { "--fix" } else { "" }

        $srcFiles = Get-ChildItem -Recurse -Filter "*.cpp" src | Select-Object -ExpandProperty FullName
        $warnings = 0
        foreach ($srcFile in $srcFiles) {
            $out = clang-tidy -p $BuildDir $fixFlag $srcFile 2>&1
            $fileWarnings = ($out | Select-String "warning:").Count
            $fileErrors   = ($out | Select-String "error:").Count
            if ($fileWarnings -gt 0 -or $fileErrors -gt 0) {
                Write-Host "  $srcFile : $fileWarnings warnings, $fileErrors errors" -ForegroundColor Yellow
                $out | Select-String "(warning|error):" | Select-Object -First 5 | ForEach-Object {
                    Write-Host "    $_" -ForegroundColor DarkYellow
                }
            }
            $warnings += $fileWarnings + $fileErrors
        }
        Write-Host "  Total clang-tidy issues: $warnings" -ForegroundColor $(if ($warnings -gt 0) { "Yellow" } else { "Green" })
        $ErrorCount += $warnings
    }
}

# ── Step 3: cppcheck ─────────────────────────────────────────
Write-Host "`n[3/3] cppcheck" -ForegroundColor White

$cppcheck = Get-Command cppcheck -ErrorAction SilentlyContinue
if (-not $cppcheck) {
    Write-Host "  ⚠  cppcheck not found" -ForegroundColor Yellow
    Write-Host "     Install: https://cppcheck.sourceforge.io/" -ForegroundColor Gray
} else {
    $out = cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem `
                    --template="{file}:{line}: [{severity}] {message}" `
                    -I include src 2>&1
    $issues = ($out | Where-Object { $_ -match "\[(error|warning|style|performance)\]" }).Count
    $out | Where-Object { $_ -match "\[" } | Select-Object -First 20 | ForEach-Object {
        Write-Host "  $_" -ForegroundColor Yellow
    }
    Write-Host "  cppcheck issues: $issues" -ForegroundColor $(if ($issues -gt 0) { "Yellow" } else { "Green" })
    $ErrorCount += $issues
}

# ── Summary ──────────────────────────────────────────────────
Write-Host "`n══════════════════════════════════════" -ForegroundColor Cyan
if ($ErrorCount -eq 0) {
    Write-Host "✔ All checks passed — 0 issues`n" -ForegroundColor Green
    exit 0
} else {
    Write-Host "✖ Total issues: $ErrorCount" -ForegroundColor Red
    Write-Host "  Run with -Fix to apply automatic fixes`n" -ForegroundColor Yellow
    exit 1
}
