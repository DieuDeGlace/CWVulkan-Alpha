# Checks the source tree before it is shared (for example posted to GitHub). It reports:
#   * game content or other binaries that must not be published (models, databases, sounds,
#     interface files, executables), outside the build and dist folders;
#   * text that looks copied from a decompiler (Ghidra's FUN_/DAT_/LAB_ names, undefined4...);
#   * absolute paths of the developer's own machine.
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File tools\check-tree.ps1
#        (exit code 1 when anything is found)
$root = Split-Path -Parent $PSScriptRoot
$skip = '\\(build[^\\]*|dist|\.git|\.vs|third_party)\\'
$files = @()
$useGit = $false
if (Get-Command git -ErrorAction SilentlyContinue) {
    $gitRoot = & git -C $root rev-parse --show-toplevel 2>$null
    $useGit = $LASTEXITCODE -eq 0 -and [IO.Path]::GetFullPath($gitRoot) -eq [IO.Path]::GetFullPath($root)
}
if ($useGit) {
    # Includes forced/staged files even when ignored, while excluding private
    # untracked handoffs and diagnostics that would not enter the repository.
    $paths = & git -C $root -c core.quotepath=false ls-files --cached --others --exclude-standard
    if ($LASTEXITCODE -ne 0) { throw 'Could not enumerate repository files.' }
    $files = @($paths | ForEach-Object { Get-Item -LiteralPath (Join-Path $root $_) -ErrorAction SilentlyContinue } |
        Where-Object { -not $_.PSIsContainer -and $_.FullName.Substring($root.Length) -notmatch $skip })
} else {
    $files = @(Get-ChildItem -Path $root -Recurse -File | Where-Object {
        $_.FullName.Substring($root.Length) -notmatch $skip -and $_.Name -notin @('CODEX-HANDOFF.md', 'CLAUDE-HANDOFF.md')
    })
}
$problems = @()

$forbidden = '\.(cub|db|plx|dat|wav|ogg|mp3|exe|dll|pdb|lib|obj|cwz|cwa|cws|cwm|rec|bin|avzd|avrg|avsp|avzc|avcr|avct|avnc|avnp|avns)$'
foreach ($f in $files) {
    if ($f.Name -match $forbidden) { $problems += "binary or game content: $($f.FullName.Substring($root.Length + 1))" }
}

$textTypes = '\.(cpp|h|c|hpp|py|md|txt|cmake|cmd|ps1|ini|glsl|vert|frag|comp)$|^CMakeLists\.txt$'
$patterns = @(
    @{ Name = 'decompiler name'; Regex = '\b(FUN|DAT|LAB|PTR|UNK)_[0-9a-fA-F]{6,}\b' },
    @{ Name = 'decompiler type'; Regex = '\bundefined[1248]\b' },
    @{ Name = 'own machine path'; Regex = '[A-Za-z]:[\\/]Users[\\/][^\\/\s]+' }
)
foreach ($f in $files) {
    if ($f.Name -notmatch $textTypes -or $f.Name -eq 'check-tree.ps1') { continue }
    $lines = @(Get-Content -LiteralPath $f.FullName -ErrorAction SilentlyContinue)
    for ($i = 0; $i -lt $lines.Count; $i++) {
        foreach ($p in $patterns) {
            if ($lines[$i] -match $p.Regex) {
                $problems += "$($p.Name): $($f.FullName.Substring($root.Length + 1)):$($i + 1): $($Matches[0])"
            }
        }
    }
}

if ($problems.Count -eq 0) {
    Write-Output 'check-tree: no prohibited content or path patterns found.'
    exit 0
}
$problems | ForEach-Object { Write-Output $_ }
Write-Output "check-tree: $($problems.Count) problem(s)."
exit 1
