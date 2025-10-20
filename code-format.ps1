<#
    code-format.ps1
    -----------------
    Formats modified CoronaEngine C++ sources using clang-format and the repo's .clang-format rules.
    Usage examples:
        ./code-format.ps1        # Format all changed C++ files (staged + unstaged)
        ./code-format.ps1 -Check # Verify formatting without modifying files
        ./code-format.ps1 -Targets src systems # Limit formatting to specific folders
#>
Param(
    [switch]$Check,
    [string[]]$Targets,
    [string[]]$Extensions
)

$ErrorActionPreference = 'Stop'

$startTimestamp = Get-Date
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$exitCode = 0
$logEntries = $null
$logPath = $null

try {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "git is required to detect modified files. Please ensure git is available in PATH."
    }

    $gitRoot = (& git -C $PSScriptRoot rev-parse --show-toplevel 2>$null)
    if (-not $gitRoot) {
        throw "Unable to determine git repository root."
    }

    $logPath = Join-Path $gitRoot 'code-format.log'
    $logEntries = [System.Collections.Generic.List[string]]::new()
    $logEntries.Add("=== code-format.ps1 run ===")
    $logEntries.Add("Started: " + $startTimestamp.ToString("yyyy-MM-dd HH:mm:ss"))
    $logEntries.Add("WorkingDirectory: $gitRoot")

    $modeLabel = 'format'
    if ($Check) { $modeLabel = 'check' }
    $logEntries.Add("Mode: $modeLabel")

    $targetLabel = '(default)'
    if ($Targets -and $Targets.Count -gt 0) { $targetLabel = $Targets -join ', ' }
    $logEntries.Add("Targets: $targetLabel")

    $extensionLabel = '*.c, *.cpp, *.cc, *.cxx, *.h, *.hpp'
    if ($Extensions -and $Extensions.Count -gt 0) { $extensionLabel = $Extensions -join ', ' }
    $logEntries.Add("Extensions: $extensionLabel")

    if (-not (Get-Command clang-format -ErrorAction SilentlyContinue)) {
        throw "clang-format not found in PATH. Please install clang-format and ensure it is available as 'clang-format'."
    }

    $clangFormatExe = 'clang-format'

    $statusLines = (& git -C $gitRoot status --porcelain --untracked=all)
    $statusCount = 0
    if ($statusLines) { $statusCount = $statusLines.Count }
    $logEntries.Add("GitStatusLines: $statusCount")

    $shouldProcess = $true

    if (-not $statusLines -or $statusLines.Count -eq 0) {
        Write-Host "No modified files detected by git."
        $logEntries.Add("Result: No modified files detected by git.")
        $shouldProcess = $false
    }

    $files = @()

    if ($shouldProcess) {
        if (-not $Extensions -or $Extensions.Count -eq 0) {
            $Extensions = @('*.cpp', '*.cc', '*.cxx', '*.h', '*.hpp')
        }

        $normalizedExtensions = $Extensions | ForEach-Object {
            if ($_ -match '^\*\.(.+)$') {
                "." + $Matches[1].ToLowerInvariant()
            }
            else {
                $_.ToLowerInvariant()
            }
        }

        $normalizedTargets = @()
        if ($Targets -and $Targets.Count -gt 0) {
            foreach ($target in $Targets) {
                if (-not $target) { continue }
                $targetPath = if ([System.IO.Path]::IsPathRooted($target)) { $target } else { Join-Path $gitRoot $target }
                if (Test-Path $targetPath) {
                    $normalizedTargets += ((Get-Item $targetPath).FullName.TrimEnd([System.IO.Path]::DirectorySeparatorChar))
                }
            }
        }

        $changedPaths = @()
        foreach ($line in $statusLines) {
            if ([string]::IsNullOrWhiteSpace($line)) { continue }
            if ($line.Length -lt 4) { continue }
            $pathPart = $line.Substring(3).Trim()
            if ($pathPart -like "* -> *") {
                $pathPart = $pathPart.Split('->')[-1].Trim()
            }
            if ($pathPart) {
                $changedPaths += $pathPart
            }
        }

        foreach ($relativePath in $changedPaths | Sort-Object -Unique) {
            $fullPath = Join-Path $gitRoot $relativePath
            if (-not (Test-Path $fullPath)) { continue }

            $extension = [System.IO.Path]::GetExtension($fullPath).ToLowerInvariant()
            if ($normalizedExtensions -notcontains $extension) { continue }

            if ($normalizedTargets.Count -gt 0) {
                $withinTarget = $false
                foreach ($targetRoot in $normalizedTargets) {
                    if ($fullPath.StartsWith($targetRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
                        $withinTarget = $true
                        break
                    }
                }
                if (-not $withinTarget) { continue }
            }

            $files += (Get-Item $fullPath)
        }

    $files = $files | Sort-Object -Property FullName -Unique
    $filteredCount = 0
    if ($files) { $filteredCount = $files.Count }
    $logEntries.Add("FilteredFiles: $filteredCount")

        if (-not $files -or $files.Count -eq 0) {
            Write-Host "No modified C++ source files matched the current selection."
            $logEntries.Add("Result: No modified C++ source files matched the current selection.")
            $shouldProcess = $false
        }
    }

    if ($shouldProcess) {
        Write-Host "Using clang-format: $clangFormatExe"
        Write-Host "Processing $($files.Count) files..."
        $logEntries.Add("ClangFormat: $clangFormatExe")
        $logEntries.Add("ProcessingCount: $($files.Count)")

        $needsFormatting = $false

        foreach ($file in $files) {
            $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
            if ($Check) {
                $formatted = & $clangFormatExe $file.FullName
                $original = Get-Content -Raw -Path $file.FullName
                if ($formatted -ne $original) {
                    Write-Host "Needs formatting: $($file.FullName)"
                    $logEntries.Add("$timestamp needs-format: $($file.FullName)")
                    $needsFormatting = $true
                }
                else {
                    $logEntries.Add("$timestamp ok: $($file.FullName)")
                }
            }
            else {
                & $clangFormatExe -i $file.FullName
                $logEntries.Add("$timestamp formatted: $($file.FullName)")
            }
        }

        if ($Check) {
            if ($needsFormatting) {
                Write-Error -Message "clang-format check failed." -ErrorAction Continue
                $logEntries.Add("Result: Check failed (needs formatting).")
                $exitCode = 1
            }
            else {
                Write-Host "All files properly formatted."
                $logEntries.Add("Result: Check passed (all files formatted).")
            }
        }
        else {
            Write-Host "Formatting complete."
            $logEntries.Add("Result: Formatting complete.")
        }
    }
}
catch {
    $exitCode = 1
    $message = $_.Exception.Message
    if (-not $logEntries) {
        $logEntries = [System.Collections.Generic.List[string]]::new()
        $logEntries.Add("=== code-format.ps1 run ===")
        $logEntries.Add("Started: " + $startTimestamp.ToString("yyyy-MM-dd HH:mm:ss"))
    }
    if (-not $logPath) {
        $fallbackRoot = if ($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
        $logPath = Join-Path $fallbackRoot 'code-format.log'
    }
    $logEntries.Add("Error: $message")
    Write-Error -Message $message -ErrorAction Continue
}
finally {
    $stopwatch.Stop()
    $completionTimestamp = Get-Date
    if (-not $logEntries) {
        $logEntries = [System.Collections.Generic.List[string]]::new()
        $logEntries.Add("=== code-format.ps1 run ===")
        $logEntries.Add("Started: " + $startTimestamp.ToString("yyyy-MM-dd HH:mm:ss"))
    }
    if (-not $logPath) {
        $logPath = Join-Path $PSScriptRoot 'code-format.log'
    }
    $logEntries.Add("Completed: " + $completionTimestamp.ToString("yyyy-MM-dd HH:mm:ss"))
    $logEntries.Add("ElapsedMs: " + [math]::Round($stopwatch.Elapsed.TotalMilliseconds, 2))
    $logEntries.Add("")
    [System.IO.File]::AppendAllLines($logPath, $logEntries)
}

exit $exitCode
