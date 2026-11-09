param(
    [Parameter(Mandatory = $true)][string]$Archive,
    [string]$BuildDirectory
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archivePath = [IO.Path]::GetFullPath($Archive)
$prefix = [IO.Path]::GetFileNameWithoutExtension($archivePath) + '/'
$hashes = [Collections.Generic.Dictionary[string, string]]::new([StringComparer]::OrdinalIgnoreCase)
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$zip = [IO.Compression.ZipFile]::OpenRead($archivePath)
$sha = [Security.Cryptography.SHA256]::Create()
try {
    $files = @($zip.Entries | Where-Object { $_.Name })
    $manifestName = $prefix + 'SHA256SUMS.txt'
    $manifest = @($files | Where-Object { $_.FullName.Replace('\', '/') -eq $manifestName })
    if ($manifest.Count -ne 1) { throw 'Expected one package-root SHA256SUMS.txt.' }
    $reader = [IO.StreamReader]::new($manifest[0].Open(), [Text.Encoding]::UTF8)
    try {
        while ($null -ne ($line = $reader.ReadLine())) {
            if ($line -notmatch '^([0-9a-f]{64})  (.+)$') { throw 'Invalid SHA-256 manifest line.' }
            $relative = $Matches[2].Replace('\', '/')
            if ($hashes.ContainsKey($relative)) { throw "Duplicate manifest path: $relative" }
            $hashes.Add($relative, $Matches[1])
        }
    } finally { $reader.Dispose() }
    foreach ($required in @('AlphaVulkan.exe', 'AlphaServer.exe', 'AlphaSetup.exe', 'AlphaVulkan.ini.example',
                            'README.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'sdk/alphavulkan_mod.h',
                            'sdk/alphavulkan_engine_module.h', 'docs/owner-content.md', 'docs/modding.md')) {
        if (-not $hashes.ContainsKey($required)) { throw "Missing required package file: $required" }
    }
    if ($files.Count -ne $hashes.Count + 1) { throw 'Archive and manifest file counts differ.' }
    foreach ($entry in $files) {
        $name = $entry.FullName.Replace('\', '/')
        if (-not $name.StartsWith($prefix, [StringComparison]::Ordinal)) { throw "Unexpected archive root: $name" }
        $relative = $name.Substring($prefix.Length)
        if ($relative -match '(^|/)\.{1,2}(/|$)|^/|:' -or -not $seen.Add($relative)) {
            throw "Invalid or duplicate archive path: $relative"
        }
        if ($relative -eq 'SHA256SUMS.txt') { continue }
        if (-not $hashes.ContainsKey($relative)) { throw "Unmanifested file: $relative" }
        if ($relative -match '(?i)(^|/)(Cube\.exe|Server\.exe|msvcr110\.dll|Save|captures|owner-cache|runtime|logs)(/|$)|\.(db|avzd|avrg|avsp|avzc|avcr|avct|avnc|avnp|avns|bin|log|dmp|rec|cwz|cwa|cws|cwm|cub|wav|png|plx|fnt)$') {
            throw "Owner content or private diagnostics in package: $relative"
        }
        $stream = $entry.Open()
        try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
        finally { $stream.Dispose() }
        if ($actual -ne $hashes[$relative]) { throw "Hash mismatch: $relative" }
    }
    if ($BuildDirectory) {
        foreach ($binary in @('AlphaVulkan.exe', 'AlphaServer.exe', 'AlphaSetup.exe')) {
            $source = Join-Path $BuildDirectory $binary
            $current = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
            if (-not $hashes.ContainsKey($binary) -or $hashes[$binary] -ne $current) { throw "Stale binary: $binary" }
        }
    }
    Write-Output "Verified $($hashes.Count) SHA-256 hashes directly in $($files.Count) archive files; no owner content/private diagnostics."
} finally {
    $sha.Dispose()
    $zip.Dispose()
}
