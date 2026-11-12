$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$compiler = Get-Command g++ -ErrorAction SilentlyContinue

if (-not $compiler) {
    Write-Error "g++ was not found. Install MinGW/MSYS2 or use Visual Studio Build Tools."
    exit 1
}

$include = Join-Path $root "include"
$sourceDir = Join-Path $root "src"
$output = Join-Path $root "foodbridge_dsa.exe"

$sources = Get-ChildItem -Path $sourceDir -Filter "*.cpp" | ForEach-Object { $_.FullName }
if (-not $sources) {
    Write-Error "No .cpp files were found in $sourceDir."
    exit 1
}

$arguments = @(
    "-std=c++14",
    "-O2",
    "-Wall",
    "-Wextra",
    "-I", $include
) + $sources + @("-o", $output)

& $compiler.Source @arguments

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Built: $output"
