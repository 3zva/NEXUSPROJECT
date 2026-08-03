param(
    [Parameter(Mandatory=$false)][string]$BuildDir = "$env:USERPROFILE\Desktop\NEXUS",
    [Parameter(Mandatory=$true)][string]$Version,
    [Parameter(Mandatory=$true)][string]$OutputDir,
    [Parameter(Mandatory=$false)][string]$MingwRoot = "C:\msys64\mingw64",
    [Parameter(Mandatory=$false)][string]$GitHubOwner = "",
    [Parameter(Mandatory=$false)][string]$GitHubRepo = "",
    [Parameter(Mandatory=$false)][string]$ReleaseTag = "",
    [Parameter(Mandatory=$false)][string]$DesktopReleaseDir = "$env:USERPROFILE\Desktop\NEXUS-GitHub-Release",
    [Parameter(Mandatory=$false)][switch]$SkipWorkerConfigUpdate,
    [Parameter(Mandatory=$false)][switch]$CommitReleaseMetadata,
    [Parameter(Mandatory=$false)][switch]$CreateGitHubRelease
)

$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$settingsPath = Join-Path $scriptDir 'release-settings.json'
$stubSource = Join-Path $scriptDir 'single_exe_stub.cpp'
$gxx = Join-Path $MingwRoot 'bin\g++.exe'
$windres = Join-Path $MingwRoot 'bin\windres.exe'

if (!(Test-Path -LiteralPath $BuildDir)) { throw "Build output folder not found: $BuildDir" }
if (!(Test-Path -LiteralPath $stubSource)) { throw "Stub source not found: $stubSource" }
if (!(Test-Path -LiteralPath $gxx)) { throw "g++ not found: $gxx" }

if (Test-Path -LiteralPath $settingsPath) {
    $settings = Get-Content -Raw -LiteralPath $settingsPath | ConvertFrom-Json
    if ([string]::IsNullOrWhiteSpace($GitHubOwner) -and $settings.github_owner) {
        $GitHubOwner = [string]$settings.github_owner
    }
    if ([string]::IsNullOrWhiteSpace($GitHubRepo) -and $settings.github_repo) {
        $GitHubRepo = [string]$settings.github_repo
    }
}
if (![string]::IsNullOrWhiteSpace($GitHubOwner) -and ![string]::IsNullOrWhiteSpace($GitHubRepo)) {
    @{
        github_owner = $GitHubOwner
        github_repo = $GitHubRepo
    } | ConvertTo-Json | Set-Content -LiteralPath $settingsPath
}
if ([string]::IsNullOrWhiteSpace($ReleaseTag)) {
    $ReleaseTag = "v$Version"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$workDir = Join-Path $OutputDir ".single-exe-work"
if (Test-Path -LiteralPath $workDir) { Remove-Item -LiteralPath $workDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $workDir | Out-Null

$stubExe = Join-Path $workDir 'NEXUS-single-stub.exe'
$resourceObject = Join-Path $workDir 'app_icon.o'
$appRc = Join-Path $repoRoot 'app.rc'
if ((Test-Path -LiteralPath $appRc) -and (Test-Path -LiteralPath $windres)) {
    & $windres $appRc -O coff -o $resourceObject
    if ($LASTEXITCODE -ne 0) { throw "Failed to compile icon resource." }
}

$compileArgs = @(
    '-std=c++17',
    '-O2',
    '-s',
    '-mwindows',
    '-municode',
    '-DWINVER=0x0A00',
    '-D_WIN32_WINNT=0x0A00',
    $stubSource
)
if (Test-Path -LiteralPath $resourceObject) {
    $compileArgs += $resourceObject
}
$compileArgs += @(
    '-o', $stubExe,
    '-lshell32',
    '-lole32',
    '-luuid'
)
& $gxx @compileArgs
if ($LASTEXITCODE -ne 0) { throw "Failed to compile the single EXE launcher." }

$releaseExe = Join-Path $OutputDir "NEXUS.exe"
if (Test-Path -LiteralPath $releaseExe) { Remove-Item -LiteralPath $releaseExe -Force }
Copy-Item -LiteralPath $stubExe -Destination $releaseExe -Force

$required = @(
    'Nexus Loader.exe',
    'configs',
    'nexus-ui',
    'nexus-client',
    'nexus-runtime-core',
    'runtime'
)
foreach ($item in $required) {
    $path = Join-Path $BuildDir $item
    if (!(Test-Path -LiteralPath $path)) { throw "Required runtime item is missing: $path" }
}

$forbiddenPatterns = @(
    '*.cpp', '*.h', '*.hpp', '*.py', '*.spec', '*.bak', '*.log',
    '*.zip', '*.nupkg', 'CMakeCache.txt', 'build.ninja'
)
foreach ($pattern in $forbiddenPatterns) {
    $found = Get-ChildItem -LiteralPath $BuildDir -Recurse -Force -Filter $pattern -ErrorAction SilentlyContinue
    if ($found) {
        $names = ($found | Select-Object -First 5 -ExpandProperty FullName) -join "`n"
        throw "Runtime folder contains forbidden development files:`n$names"
    }
}

$files = Get-ChildItem -LiteralPath $BuildDir -Recurse -File |
    Sort-Object FullName
$buildRootFull = (Resolve-Path -LiteralPath $BuildDir).Path.TrimEnd('\') + '\'
$buildRootUri = [System.Uri]::new($buildRootFull)

$archivePath = Join-Path $workDir 'payload.bin'
$archiveStream = [System.IO.File]::Open($archivePath, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write)
try {
    function Write-Bytes([System.IO.Stream]$Stream, [byte[]]$Bytes) {
        $Stream.Write($Bytes, 0, $Bytes.Length)
    }
    function Write-U16([System.IO.Stream]$Stream, [int]$Value) {
        $bytes = New-Object byte[] 2
        $bytes[0] = [byte]($Value -band 0xff)
        $bytes[1] = [byte](($Value -shr 8) -band 0xff)
        Write-Bytes $Stream $bytes
    }
    function Write-U32([System.IO.Stream]$Stream, [int64]$Value) {
        $bytes = New-Object byte[] 4
        for ($i = 0; $i -lt 4; $i++) {
            $bytes[$i] = [byte](($Value -shr (8 * $i)) -band 0xff)
        }
        Write-Bytes $Stream $bytes
    }
    function Write-U64([System.IO.Stream]$Stream, [int64]$Value) {
        $bytes = New-Object byte[] 8
        for ($i = 0; $i -lt 8; $i++) {
            $bytes[$i] = [byte](($Value -shr (8 * $i)) -band 0xff)
        }
        Write-Bytes $Stream $bytes
    }

    Write-Bytes $archiveStream ([System.Text.Encoding]::ASCII.GetBytes('NEXUSPKG01'))
    Write-U32 $archiveStream $files.Count

    foreach ($file in $files) {
        $fileUri = [System.Uri]::new($file.FullName)
        $relative = [System.Uri]::UnescapeDataString($buildRootUri.MakeRelativeUri($fileUri).ToString())
        $pathBytes = [System.Text.Encoding]::UTF8.GetBytes($relative)
        if ($pathBytes.Length -gt 65535) { throw "Packaged path is too long: $relative" }
        Write-U16 $archiveStream $pathBytes.Length
        Write-U64 $archiveStream $file.Length
        Write-Bytes $archiveStream $pathBytes

        $input = [System.IO.File]::OpenRead($file.FullName)
        try {
            $input.CopyTo($archiveStream)
        } finally {
            $input.Dispose()
        }
    }
} finally {
    $archiveStream.Dispose()
}

$archiveSize = (Get-Item -LiteralPath $archivePath).Length
$releaseStream = [System.IO.File]::Open($releaseExe, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
try {
    $payloadStream = [System.IO.File]::OpenRead($archivePath)
    try {
        $payloadStream.CopyTo($releaseStream)
    } finally {
        $payloadStream.Dispose()
    }

    Write-U64 $releaseStream $archiveSize
    Write-Bytes $releaseStream ([System.Text.Encoding]::ASCII.GetBytes('NEXUSPKGEND01'))
} finally {
    $releaseStream.Dispose()
}

$hash = (Get-FileHash -LiteralPath $releaseExe -Algorithm SHA256).Hash.ToLowerInvariant()
$size = (Get-Item -LiteralPath $releaseExe).Length
$assetName = Split-Path -Leaf $releaseExe
$githubDownloadUrl = ""
if (![string]::IsNullOrWhiteSpace($GitHubOwner) -and ![string]::IsNullOrWhiteSpace($GitHubRepo)) {
    $githubDownloadUrl = "https://github.com/$GitHubOwner/$GitHubRepo/releases/latest/download/$assetName"
}
$readmeDownloadUrl = $githubDownloadUrl
if ([string]::IsNullOrWhiteSpace($readmeDownloadUrl)) {
    $readmeDownloadUrl = 'Not configured. Re-run the build with -GitHubOwner YOUR_ACCOUNT -GitHubRepo YOUR_REPO.'
}
@{
    version = $Version
    asset = $assetName
    sha256 = $hash
    size = $size
    bundled_files = $files.Count
    github_download_url = $githubDownloadUrl
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDir 'single-exe-release-values.json')

Remove-Item -LiteralPath $workDir -Recurse -Force

if (!$SkipWorkerConfigUpdate) {
    if ([string]::IsNullOrWhiteSpace($githubDownloadUrl)) {
        throw "GitHub owner/repo are required to update Worker release URLs. Re-run with -GitHubOwner YOUR_ACCOUNT -GitHubRepo YOUR_REPO once, or pass -SkipWorkerConfigUpdate."
    }

    $wranglerPath = Join-Path $repoRoot 'cloudflare-worker\wrangler.jsonc'
    if (!(Test-Path -LiteralPath $wranglerPath)) { throw "wrangler.jsonc not found: $wranglerPath" }
    function Set-JsonStringValue([string]$Text, [string]$Key, [string]$Value) {
        $pattern = '("' + [regex]::Escape($Key) + '"\s*:\s*")[^"]*(")'
        $escapedValue = $Value.Replace('\', '\\').Replace('"', '\"')
        return [regex]::Replace(
            $Text,
            $pattern,
            { param($match) $match.Groups[1].Value + $escapedValue + $match.Groups[2].Value },
            1
        )
    }
    $wranglerText = Get-Content -Raw -LiteralPath $wranglerPath
    $wranglerText = Set-JsonStringValue $wranglerText 'LATEST_VERSION' $Version
    $wranglerText = Set-JsonStringValue $wranglerText 'LATEST_PACKAGE_URL' $githubDownloadUrl
    $wranglerText = Set-JsonStringValue $wranglerText 'LATEST_PACKAGE_SHA256' $hash
    $wranglerText = Set-JsonStringValue $wranglerText 'LATEST_PACKAGE_SIZE' ([string]$size)
    $wranglerText = Set-JsonStringValue $wranglerText 'SETUP_DOWNLOAD_URL' $githubDownloadUrl
    $wranglerText | ConvertFrom-Json | Out-Null
    Set-Content -LiteralPath $wranglerPath -Value $wranglerText -NoNewline
}

if ($CreateGitHubRelease) {
    if ([string]::IsNullOrWhiteSpace($githubDownloadUrl)) {
        throw "GitHub owner/repo are required to create a GitHub Release."
    }
    $gh = Get-Command gh -ErrorAction SilentlyContinue
    if (!$gh) {
        $ghCandidates = @(
            "$env:ProgramFiles\GitHub CLI\gh.exe",
            "$env:LOCALAPPDATA\Programs\GitHub CLI\gh.exe"
        )
        foreach ($candidate in $ghCandidates) {
            if (Test-Path -LiteralPath $candidate) {
                $gh = [pscustomobject]@{ Source = $candidate }
                break
            }
        }
    }
    if (!$gh) {
        throw "GitHub CLI was not found. Install GitHub CLI and run 'gh auth login', then re-run with -CreateGitHubRelease."
    }
    & $gh.Source auth status *> $null
    if ($LASTEXITCODE -ne 0) {
        throw "GitHub CLI is not authenticated. Run 'gh auth login', then re-run with -CreateGitHubRelease."
    }

    $repo = "$GitHubOwner/$GitHubRepo"
    $releaseTitle = "NEXUS $Version"
    $releaseNotes = @"
NEXUS standalone release $Version

Asset: $assetName
SHA256: $hash
Size: $size bytes
"@
    $releaseExists = $false
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $gh.Source release view $ReleaseTag --repo $repo 1>$null 2>$null
    $releaseViewExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorActionPreference
    if ($releaseViewExitCode -eq 0) {
        $releaseExists = $true
    }
    if ($releaseExists) {
        & $gh.Source release upload $ReleaseTag $releaseExe --repo $repo --clobber
    } else {
        & $gh.Source release create $ReleaseTag $releaseExe --repo $repo --title $releaseTitle --notes $releaseNotes
    }
    if ($LASTEXITCODE -ne 0) {
        throw "GitHub Release publish failed."
    }
}

if (![string]::IsNullOrWhiteSpace($DesktopReleaseDir)) {
    New-Item -ItemType Directory -Force -Path $DesktopReleaseDir | Out-Null
    Get-ChildItem -LiteralPath $DesktopReleaseDir -Force -File -ErrorAction SilentlyContinue | Remove-Item -Force
    Copy-Item -LiteralPath $releaseExe -Destination (Join-Path $DesktopReleaseDir $assetName) -Force
    Copy-Item -LiteralPath (Join-Path $OutputDir 'single-exe-release-values.json') -Destination (Join-Path $DesktopReleaseDir 'single-exe-release-values.json') -Force
    @"
NEXUS GitHub release upload folder

Upload this file to the GitHub Release:
$assetName

Release tag:
$ReleaseTag

Download URL:
$readmeDownloadUrl

Cloudflare Worker config:
cloudflare-worker\wrangler.jsonc

After uploading the EXE to GitHub, deploy the Worker:
cd cloudflare-worker
npx wrangler deploy
"@ | Set-Content -LiteralPath (Join-Path $DesktopReleaseDir 'README-UPLOAD.txt')
}

Write-Host "Single EXE: $releaseExe"
Write-Host "SHA256: $hash"
Write-Host "Size: $size"
Write-Host "Bundled files: $($files.Count)"
if ($githubDownloadUrl) { Write-Host "GitHub URL: $githubDownloadUrl" }
if (!$SkipWorkerConfigUpdate) { Write-Host "Updated: cloudflare-worker\wrangler.jsonc" }
if ($DesktopReleaseDir) { Write-Host "Desktop release folder: $DesktopReleaseDir" }

if ($CommitReleaseMetadata) {
    Push-Location $repoRoot
    try {
        $trackedChanges = git status --porcelain -- cloudflare-worker/wrangler.jsonc
        if ($trackedChanges) {
            git add cloudflare-worker/wrangler.jsonc
            git commit -m "Update release metadata for $ReleaseTag"
            if ($LASTEXITCODE -ne 0) {
                throw "Git commit failed."
            }
            Write-Host "Committed release metadata: $ReleaseTag"
        } else {
            Write-Host "No release metadata changes to commit."
        }
    } finally {
        Pop-Location
    }
}
