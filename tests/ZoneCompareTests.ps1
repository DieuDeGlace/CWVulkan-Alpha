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

# This named zone starts in water. Mutate its recorded output and scratch
# independently to exercise the same strict comparison the corpus uses.
$waterFile = Join-Path $scratch 'generated-water.avzd'
Invoke-Comparison 'record-water' @('--record', '41', '32644', '32657', $waterFile) $true
$waterBytes = [IO.File]::ReadAllBytes($waterFile)
$firstCount = [BitConverter]::ToUInt32($waterBytes, 28)
$waterOffset = 32
while ($waterOffset -lt 32 + 4 * $firstCount -and ($waterBytes[$waterOffset + 3] -band 31) -ne 2) {
    $waterOffset += 4
}
if ($waterOffset -ge 32 + 4 * $firstCount) { throw 'Expected stored water in the first column.' }
$corpus = Join-Path $scratch 'water-corpus'
New-Item -ItemType Directory -Path $corpus -Force | Out-Null
foreach ($lane in 0, 1, 2) {
    $changed = [byte[]]$waterBytes.Clone()
    $offset = $waterOffset + $lane
    $changed[$offset] = $changed[$offset] -bxor 1
    $path = Join-Path $corpus "water-lane-$lane.avzd"
    [IO.File]::WriteAllBytes($path, $changed)
    Invoke-Comparison "water-lane-$lane" @('--strict', $path) ($lane -ne 0)
    if (!(Select-String -LiteralPath (Join-Path $scratch "water-lane-$lane.log") -SimpleMatch 'scratch differs from committed light: 1 of' -Quiet)) {
        throw "Missing recorded water scratch difference for lane $lane."
    }
}
$log = Join-Path $scratch 'water-corpus.log'
& (Join-Path $PSScriptRoot '../tools/CompareCorpus.ps1') -Recordings $corpus -ZoneCompare $ZoneCompare -OutputLog $log -BatchSize 2
if (!(Select-String -LiteralPath $log -SimpleMatch 'Zones with differences: 1.' -Quiet) -or
    !(Select-String -LiteralPath $log -SimpleMatch 'differed from committed light in 3 of' -Quiet)) {
    throw 'The corpus summary did not distinguish committed water light from scratch.'
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
