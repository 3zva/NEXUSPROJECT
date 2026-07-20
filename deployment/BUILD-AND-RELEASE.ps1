param(
    [Parameter(Mandatory=$false)][string]$BuildDir = "$env:USERPROFILE\Desktop\NEXUS",
    [Parameter(Mandatory=$true)][string]$Version,
    [Parameter(Mandatory=$true)][string]$OutputDir
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$packageDir = Join-Path $OutputDir "NEXUS-App-$Version"
if (Test-Path $packageDir) { Remove-Item $packageDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null

$required = @(
    'Nexus Loader.exe',
    'configs',
    'nexus-ui',
    'recoil-ui',
    'recoilmaster-main',
    'runtime'
)

foreach ($item in $required) {
    $source = Join-Path $BuildDir $item
    if (!(Test-Path -LiteralPath $source)) {
        throw "Required runtime item is missing: $source"
    }
    Copy-Item -LiteralPath $source -Destination $packageDir -Recurse -Force
}

$forbiddenPatterns = @(
    '*.cpp', '*.h', '*.hpp', '*.py', '*.spec', '*.bak', '*.log',
    '*.zip', '*.nupkg', 'CMakeCache.txt', 'build.ninja'
)
foreach ($pattern in $forbiddenPatterns) {
    $found = Get-ChildItem -LiteralPath $packageDir -Recurse -Force -Filter $pattern -ErrorAction SilentlyContinue
    if ($found) {
        $names = ($found | Select-Object -First 5 -ExpandProperty FullName) -join "`n"
        throw "Package contains forbidden development files:`n$names"
    }
}

$zip = Join-Path $OutputDir "NEXUS-App-$Version.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $zip -CompressionLevel Optimal
$hash = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLowerInvariant()
$size = (Get-Item $zip).Length

@{
    version = $Version
    package = $zip
    sha256 = $hash
    size = $size
} | ConvertTo-Json | Set-Content (Join-Path $OutputDir 'release-values.json')

Write-Host "Package: $zip"
Write-Host "SHA256: $hash"
Write-Host "Size: $size"
