param([Parameter(Mandatory = $true)][string]$ZoneCompare)

$ErrorActionPreference = 'Stop'
$scratch = Join-Path (Split-Path -Parent $ZoneCompare) 'ZoneCompareTests'
New-Item -ItemType Directory -Path $scratch -Force | Out-Null

function Invoke-Comparison([string]$Name, [string[]]$ToolArguments, [bool]$ShouldPass) {
    # Windows PowerShell represents redirected native stderr as error records;
    # expected rejection diagnostics must not interrupt checking the exit code.
    $ErrorActionPreference = 'Continue'
    & $ZoneCompare @ToolArguments > (Join-Path $scratch "$Name.log") 2>&1
    $result = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if (($result -eq 0) -ne $ShouldPass) {
        throw "$Name returned $result; expected success=$ShouldPass. See $scratch."
    }
    Write-Output "$Name passed"
}

$recording = Join-Path $scratch 'generated.avzd'
Invoke-Comparison 'record' @('--record', '41', '32803', '32804', $recording) $true
Invoke-Comparison 'equal' @('--strict', $recording) $true

# These are generated diagnostic files, never reference-game fixtures. Changing
# one compared field must turn a successful comparison into a failed command.
$original = [System.IO.File]::ReadAllBytes($recording)
$solidOffset = -1
$lightOffset = -1
$columnOffset = 24
for ($column = 0; $column -lt 65536 -and ($solidOffset -lt 0 -or $lightOffset -lt 0); $column++) {
    $count = [BitConverter]::ToUInt32($original, $columnOffset + 4)
    $blockOffset = $columnOffset + 8
    for ($block = 0; $block -lt $count -and ($solidOffset -lt 0 -or $lightOffset -lt 0); $block++) {
        $offset = $blockOffset + 4 * $block
        $kind = $original[$offset + 3] -band 31
        if ($kind -eq 0 -or $kind -eq 2) {
            if ($lightOffset -lt 0) {
                $lightOffset = $offset
                if ($kind -eq 2) { $lightOffset += 2 }
            }
        } elseif ($solidOffset -lt 0) {
            $solidOffset = $offset
        }
    }
    $columnOffset = $blockOffset + 4 * $count
}
if ($solidOffset -lt 0 -or $lightOffset -lt 0) {
    throw 'Generated fixture must contain both a solid cell and stored air/water.'
}

foreach ($change in @(@('solid', $solidOffset), @('light', $lightOffset))) {
    $changed = [byte[]]$original.Clone()
    $changed[$change[1]] = $changed[$change[1]] -bxor 1
    $path = Join-Path $scratch "$($change[0]).avzd"
    [System.IO.File]::WriteAllBytes($path, $changed)
    Invoke-Comparison $change[0] @($path, '--strict') $false
    Invoke-Comparison "$($change[0])-report" @($path) $true
}

$trailing = Join-Path $scratch 'trailing.avzd'
$withTrailingByte = [byte[]]::new($original.Length + 1)
$original.CopyTo($withTrailingByte, 0)
[System.IO.File]::WriteAllBytes($trailing, $withTrailingByte)
Invoke-Comparison 'trailing' @('--strict', $trailing) $false
$truncated = Join-Path $scratch 'truncated.avzd'
[System.IO.File]::WriteAllBytes($truncated, [byte[]]$original[0..22])
Invoke-Comparison 'truncated' @('--strict', $truncated) $false
Invoke-Comparison 'missing' @('--strict', (Join-Path $scratch 'missing.avzd')) $false
Invoke-Comparison 'empty' @('--strict') $false
Invoke-Comparison 'unknown-option' @('--strcit', $recording) $false

$spawnRecording = Join-Path $scratch 'generated.avsp'
Invoke-Comparison 'record-spawns' @('--record', '41', '32803', '32804', $spawnRecording) $true
Invoke-Comparison 'spawns-v2' @('--strict', $spawnRecording) $true
Invoke-Comparison 'spawns-prefix' @('--strict', '--prefix', $spawnRecording) $true
$spawnBytes = [System.IO.File]::ReadAllBytes($spawnRecording)
$spawnCount = [BitConverter]::ToUInt32($spawnBytes, 24)
if ($spawnCount -eq 0) { throw 'Spawn diagnostic fixture must have at least one prefix record.' }

# Appending a retained suffix changes a full-list comparison, while an explicit
# metadata-backed prefix comparison still verifies the same baseline records.
$withSuffix = [byte[]]::new($spawnBytes.Length + 0x10f0)
$spawnBytes.CopyTo($withSuffix, 0)
[Buffer]::BlockCopy($spawnBytes, 32, $withSuffix, $spawnBytes.Length, 0x10f0)
[BitConverter]::GetBytes([uint32]($spawnCount + 1)).CopyTo($withSuffix, 24)
$suffixFile = Join-Path $scratch 'suffix.avsp'
[System.IO.File]::WriteAllBytes($suffixFile, $withSuffix)
Invoke-Comparison 'suffix-full' @('--strict', $suffixFile) $false
Invoke-Comparison 'suffix-prefix' @('--strict', '--prefix', $suffixFile) $true

$changedPrefix = [byte[]]$withSuffix.Clone()
$changedPrefix[32 + 0x2c] = $changedPrefix[32 + 0x2c] -bxor 1
$changedFile = Join-Path $scratch 'changed-prefix.avsp'
[System.IO.File]::WriteAllBytes($changedFile, $changedPrefix)
Invoke-Comparison 'changed-prefix' @('--strict', '--prefix', $changedFile) $false

$invalidPrefix = [byte[]]$spawnBytes.Clone()
[BitConverter]::GetBytes([uint32]($spawnCount + 1)).CopyTo($invalidPrefix, 28)
$invalidFile = Join-Path $scratch 'invalid-prefix.avsp'
[System.IO.File]::WriteAllBytes($invalidFile, $invalidPrefix)
Invoke-Comparison 'invalid-prefix' @('--strict', '--prefix', $invalidFile) $false

$legacy = [byte[]]::new($spawnBytes.Length - 4)
[Buffer]::BlockCopy($spawnBytes, 0, $legacy, 0, 28)
[Buffer]::BlockCopy($spawnBytes, 32, $legacy, 28, $spawnBytes.Length - 32)
[BitConverter]::GetBytes([uint32]1).CopyTo($legacy, 4)
$legacyFile = Join-Path $scratch 'legacy.avsp'
[System.IO.File]::WriteAllBytes($legacyFile, $legacy)
Invoke-Comparison 'legacy-full' @('--strict', $legacyFile) $true
Invoke-Comparison 'legacy-prefix-rejected' @('--strict', '--prefix', $legacyFile) $false
Invoke-Comparison 'wrong-prefix-format' @('--strict', '--prefix', $recording) $false

$shortSuffix = [byte[]]::new($withSuffix.Length - 1)
[Buffer]::BlockCopy($withSuffix, 0, $shortSuffix, 0, $shortSuffix.Length)
$shortSuffixFile = Join-Path $scratch 'short-suffix.avsp'
[System.IO.File]::WriteAllBytes($shortSuffixFile, $shortSuffix)
Invoke-Comparison 'short-suffix' @('--strict', '--prefix', $shortSuffixFile) $false
