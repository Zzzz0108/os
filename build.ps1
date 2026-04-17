param(
    [ValidateSet("all", "cmd_demo", "mem_test", "mem_test_compare", "file_main", "create_disk", "clean")]
    [string]$Target = "all",
    [string]$Compiler = "gcc"
)

$ErrorActionPreference = "Stop"

function Assert-Compiler([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Compiler '$Name' not found in PATH."
    }
}

function Build-Target {
    param(
        [string]$Name,
        [string]$Output,
        [string[]]$Sources
    )

    if (Test-Path $Output) {
        try {
            Remove-Item $Output -Force -ErrorAction Stop
        }
        catch {
            $procName = [System.IO.Path]::GetFileNameWithoutExtension($Output)
            throw "Cannot overwrite '$Output'. It may be in use by a running process ('$procName'). Please stop it and retry (example: Stop-Process -Name $procName -Force)."
        }
    }

    Write-Host "[BUILD] $Name -> $Output"
    & $Compiler @CommonFlags @Sources -o $Output
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed: $Name"
    }
}

Assert-Compiler -Name $Compiler

$CommonFlags = @(
    "-finput-charset=UTF-8",
    "-fexec-charset=UTF-8",
    "-Iinc"
)

$CoreSources = @(
    "src/cmd/cmd.c",
    "src/file/file_myfs.c",
    "src/Memory/mem.c",
    "src/Memory/mem_sync_win.c",
    "src/process/process.c",
    "src/process/process_queue.c",
    "src/process/process_scheduler.c",
    "src/process/process_interface.c"
)

$PlatformSources = @(
    "src/platform/win/platform_console_win.c",
    "src/platform/win/platform_time_win.c",
    "src/platform/win/platform_thread_win.c"
)

$TargetMap = @{
    "cmd_demo" = @{
        Output  = "cmd_demo.exe"
        Sources = @("src/cmd/cmd_demo.c") + $CoreSources + $PlatformSources
    }
    "mem_test" = @{
        Output  = "mem_test.exe"
        Sources = @("src/Memory/mem_test.c") + $CoreSources + $PlatformSources
    }
    "mem_test_compare" = @{
        Output  = "mem_test_compare.exe"
        Sources = @("src/Memory/mem_test_compare.c") + $CoreSources + $PlatformSources
    }
    "file_main" = @{
        Output  = "file_main.exe"
        Sources = @("src/file/file_main.c") + $CoreSources + $PlatformSources
    }
    "create_disk" = @{
        Output  = "create_disk.exe"
        Sources = @("src/file/file_create_disk.c") + $CoreSources + $PlatformSources
    }
}

if ($Target -eq "clean") {
    Write-Host "[CLEAN] removing generated executables"
    foreach ($exe in @("cmd_demo.exe", "mem_test.exe", "mem_test_compare.exe", "file_main.exe", "create_disk.exe")) {
        if (Test-Path $exe) {
            Remove-Item $exe -Force
            Write-Host "  removed $exe"
        }
    }
    exit 0
}

$BuildList = if ($Target -eq "all") {
    @("cmd_demo", "mem_test", "mem_test_compare", "file_main", "create_disk")
} else {
    @($Target)
}

foreach ($item in $BuildList) {
    $cfg = $TargetMap[$item]
    Build-Target -Name $item -Output $cfg.Output -Sources $cfg.Sources
}

Write-Host "[DONE] build completed successfully"
