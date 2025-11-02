<#
    code-tidy.ps1
    -----------------
    Runs clang-tidy on CoronaEngine C/C++ sources using the repo's .clang-tidy configuration.
    Usage examples:
        ./code-tidy.ps1                # Check git-modified files
        ./code-tidy.ps1 -All           # Check all tracked C/C++ files in default targets
        ./code-tidy.ps1 -Fix           # Apply suggested fixes in-place
        ./code-tidy.ps1 -BuildDir build/Release # Use a custom build directory for compile_commands.json
#>
Param(
    [switch]$Fix,
    [switch]$All,
    [string[]]$Targets,
    [string[]]$Extensions,
    [string]$BuildDir
)

$ErrorActionPreference = 'Stop'

$startTimestamp = Get-Date
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$exitCode = 0
$logEntries = $null
$logPath = $null

try {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "git is required to detect source files. Please ensure git is available in PATH."
    }

    if (-not (Get-Command clang-tidy -ErrorAction SilentlyContinue)) {
        throw "clang-tidy not found in PATH. Please install clang-tidy and ensure it is available as 'clang-tidy'."
    }

    $gitRoot = (& git -C $PSScriptRoot rev-parse --show-toplevel 2>$null)
    if (-not $gitRoot) {
        throw "Unable to determine git repository root."
    }

    $logPath = Join-Path $gitRoot 'code-tidy.log'
    $logEntries = [System.Collections.Generic.List[string]]::new()
    $logEntries.Add("=== code-tidy.ps1 run ===")
    $logEntries.Add("Started: " + $startTimestamp.ToString("yyyy-MM-dd HH:mm:ss"))
    $logEntries.Add("WorkingDirectory: $gitRoot")

    $modeLabel = 'check'
    if ($Fix) { $modeLabel = 'fix' }
    $logEntries.Add("Mode: $modeLabel")

    $scopeLabel = 'git-modified'
    if ($All) { $scopeLabel = 'all' }
    $logEntries.Add("Scope: $scopeLabel")

    $defaultTargets = @('engine', 'examples', 'include', 'src', 'tests')
    $effectiveTargets = $Targets
    if (-not $effectiveTargets -or $effectiveTargets.Count -eq 0) {
        $effectiveTargets = $defaultTargets
    }

    $targetLabel = '(none)'
    if ($effectiveTargets -and $effectiveTargets.Count -gt 0) { $targetLabel = $effectiveTargets -join ', ' }
    $logEntries.Add("Targets: $targetLabel")

    $extensionLabel = '*.c, *.cpp, *.cc, *.cxx, *.h, *.hpp'
    if ($Extensions -and $Extensions.Count -gt 0) { $extensionLabel = $Extensions -join ', ' }
    $logEntries.Add("Extensions: $extensionLabel")

    $buildRoot = $BuildDir
    if (-not $buildRoot) { $buildRoot = 'build' }
    if (-not [System.IO.Path]::IsPathRooted($buildRoot)) {
        $buildRoot = Join-Path $gitRoot $buildRoot
    }

    $compileCommandsPath = Join-Path $buildRoot 'compile_commands.json'
    if (-not (Test-Path $compileCommandsPath)) {
        throw "compile_commands.json not found at $compileCommandsPath. Configure the project with CMake using -DCMAKE_EXPORT_COMPILE_COMMANDS=ON."
    }

    $clangTidyExe = 'clang-tidy'

    if (-not $Extensions -or $Extensions.Count -eq 0) {
        $Extensions = @('*.c', '*.cpp', '*.cc', '*.cxx', '*.h', '*.hpp')
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
    foreach ($target in $effectiveTargets) {
        if (-not $target) { continue }
        $targetPath = if ([System.IO.Path]::IsPathRooted($target)) { $target } else { Join-Path $gitRoot $target }
        if (Test-Path $targetPath) {
            $normalizedTargets += ((Get-Item $targetPath).FullName.TrimEnd([System.IO.Path]::DirectorySeparatorChar))
        }
    }

    $files = @()

    if ($All) {
        $searchRoots = @()
        if ($normalizedTargets -and $normalizedTargets.Count -gt 0) {
            $searchRoots = $normalizedTargets
        }
        else {
            foreach ($defaultTarget in $defaultTargets) {
                $searchRoots += (Join-Path $gitRoot $defaultTarget)
            }
        }

        $rootCount = 0
        if ($searchRoots) { $rootCount = $searchRoots.Count }
        $logEntries.Add("SearchRootCount: $rootCount")

        foreach ($root in $searchRoots) {
            if (-not (Test-Path $root)) { continue }
            $files += Get-ChildItem -Path $root -Recurse -File | Where-Object {
                $normalizedExtensions -contains ($_.Extension.ToLowerInvariant())
            }
        }
    }
    else {
        $statusLines = (& git -C $gitRoot status --porcelain)
        $statusCount = 0
        if ($statusLines) { $statusCount = $statusLines.Count }
        $logEntries.Add("GitStatusLines: $statusCount")

        if ($statusLines) {
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
        }
    }

    $files = $files | Sort-Object -Property FullName -Unique
    $fileCount = 0
    if ($files) { $fileCount = $files.Count }
    $logEntries.Add("FilesToProcess: $fileCount")

    if (-not $files -or $files.Count -eq 0) {
        Write-Host "No C/C++ files matched the current selection."
    }
    else {
        Write-Host "Using clang-tidy: $clangTidyExe"
        Write-Host "Processing $($files.Count) files..."

        foreach ($file in $files) {
            $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
            $tidyArgs = @($file.FullName, "-p", $buildRoot)
            if ($Fix) {
                $tidyArgs += '--fix'
                $tidyArgs += '--fix-errors'
            }

            & $clangTidyExe @tidyArgs
            $tidyExit = $LASTEXITCODE

            if ($tidyExit -ne 0) {
                $exitCode = $tidyExit
                $logEntries.Add("$timestamp tidy-error($tidyExit): $($file.FullName)")
            }
            else {
                if ($Fix) {
                    $logEntries.Add("$timestamp tidy-fixed: $($file.FullName)")
                }
                else {
                    $logEntries.Add("$timestamp tidy-ok: $($file.FullName)")
                }
            }
        }
    }
}
catch {
    $exitCode = 1
    $message = $_.Exception.Message
    if (-not $logEntries) {
        $logEntries = [System.Collections.Generic.List[string]]::new()
        $logEntries.Add("=== code-tidy.ps1 run ===")
        $logEntries.Add("Started: " + $startTimestamp.ToString("yyyy-MM-dd HH:mm:ss"))
    }
    if (-not $logPath) {
        $fallbackRoot = if ($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
        $logPath = Join-Path $fallbackRoot 'code-tidy.log'
    }
    $logEntries.Add("Error: $message")
    Write-Error -Message $message -ErrorAction Continue
}
finally {
    $stopwatch.Stop()
    $completionTimestamp = Get-Date
    if (-not $logEntries) {
        $logEntries = [System.Collections.Generic.List[string]]::new()
        $logEntries.Add("=== code-tidy.ps1 run ===")
        $logEntries.Add("Started: " + $startTimestamp.ToString("yyyy-MM-dd HH:mm:ss"))
    }
    if (-not $logPath) {
        $logPath = Join-Path $PSScriptRoot 'code-tidy.log'
    }
    $logEntries.Add("Completed: " + $completionTimestamp.ToString("yyyy-MM-dd HH:mm:ss"))
    $logEntries.Add("ElapsedMs: " + [math]::Round($stopwatch.Elapsed.TotalMilliseconds, 2))
    $logEntries.Add("")
    [System.IO.File]::AppendAllLines($logPath, $logEntries)
}

exit $exitCode
