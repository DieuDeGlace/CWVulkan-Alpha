param([string]$BuildDirectory = $env:ALPHAVULKAN_BUILD_DIR)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $BuildDirectory) {
    $BuildDirectory = if (Test-Path -LiteralPath (Join-Path $projectRoot 'build-codex\build.ninja')) { 'build-codex' } else { 'build' }
}
$buildRoot = if ([IO.Path]::IsPathRooted($BuildDirectory)) { [IO.Path]::GetFullPath($BuildDirectory) } else {
    [IO.Path]::GetFullPath((Join-Path $projectRoot $BuildDirectory))
}
$distributionRoot = Join-Path $projectRoot 'dist'
$name = 'AlphaVulkan-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$outputRoot = Join-Path $distributionRoot $name
if (Test-Path -LiteralPath $outputRoot) { throw 'Package destination already exists.' }

# An allowlist and a fresh directory keep private caches/captures/saves out of
# packages. Existing distributions are retained; this script never deletes them.
$files = [ordered]@{}
foreach ($binary in @('AlphaVulkan.exe', 'AlphaServer.exe', 'AlphaSetup.exe')) {
    $files[$binary] = Join-Path $buildRoot $binary
}
$files['AlphaVulkan.ini.example'] = Join-Path $projectRoot 'config\AlphaVulkan.ini'
foreach ($file in @('TimeLapse.dll', 'mod.ini')) {
    $files["Mods\TimeLapse\$file"] = Join-Path $buildRoot "Mods\TimeLapse\$file"
}
foreach ($file in @('alphavulkan_mod.h', 'alphavulkan_engine_module.h')) {
    $files["sdk\$file"] = Join-Path $projectRoot "sdk\$file"
}
foreach ($file in @('modding.md', 'owner-content.md', 'water-extension.md', 'controller-support.md', 'verification.md', 'roadmap.md',
                   'CLEANROOM.md', 'CLEANROOM_AUTHOR.md', 'beta-release.md')) {
    $files["docs\$file"] = Join-Path $projectRoot "docs\$file"
}
foreach ($file in @('README.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md')) {
    $files[$file] = Join-Path $projectRoot $file
}
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $projectRoot 'licenses') -File) {
    $files["licenses\$($file.Name)"] = $file.FullName
}
foreach ($relative in @('mods\TimeLapse\TimeLapse.c', 'mods\TimeLapse\mod.ini', 'modules\StoneTint\StoneTint.cpp')) {
    $files["examples\$relative"] = Join-Path $projectRoot "examples\$relative"
}
foreach ($source in $files.Values) {
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Missing package input: $source" }
}
New-Item -ItemType Directory -Path $outputRoot | Out-Null
foreach ($relative in $files.Keys) {
    $destination = Join-Path $outputRoot $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $files[$relative] -Destination $destination
}
$manifest = foreach ($relative in $files.Keys) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $outputRoot $relative) -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $relative"
}
[IO.File]::WriteAllLines((Join-Path $outputRoot 'SHA256SUMS.txt'), $manifest, [Text.UTF8Encoding]::new($false))
$archive = Join-Path $distributionRoot ($name + '.zip')
Compress-Archive -LiteralPath $outputRoot -DestinationPath $archive
Write-Output "Packaged $archive ($($files.Count) allowlisted files plus SHA256SUMS.txt)."
