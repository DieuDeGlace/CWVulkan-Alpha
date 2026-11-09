param(
    [Parameter(Mandatory)][string]$Recordings,
    [Parameter(Mandatory)][string]$ZoneCompare,
    [Parameter(Mandatory)][string]$OutputLog,
    [ValidateRange(1, 100)][int]$BatchSize = 20
)
$ErrorActionPreference = 'Stop'
$comparator = (Resolve-Path -LiteralPath $ZoneCompare).Path
$files = @(Get-ChildItem -LiteralPath $Recordings -Recurse -File -Filter '*.avzd' | Sort-Object FullName)
if (-not $files.Count) { throw 'No zone recordings found.' }
$log = [IO.Path]::GetFullPath($OutputLog)
New-Item -ItemType Directory -Path (Split-Path -Parent $log) -Force | Out-Null
[IO.File]::WriteAllText($log, "Native zone corpus: $($files.Count) files`n")
for ($first = 0; $first -lt $files.Count; $first += $BatchSize) {
    $last = [Math]::Min($first + $BatchSize - 1, $files.Count - 1)
    $arguments = @($files[$first..$last] | ForEach-Object FullName)
    # Strict native mismatches intentionally produce stderr and exit 1. Preserve
    # those diagnostics without promoting them to a PowerShell parsing failure.
    $ErrorActionPreference = 'Continue'
    $output = @(& $comparator --strict @arguments 2>&1 | ForEach-Object { $_.ToString() })
    $code = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    [IO.File]::AppendAllText($log, ($output -join "`n") + "`nBatch $first..$last exit $code`n")
}
[long]$same = 0
[long]$total = 0
$zones = 0
$exact = 0
foreach ($line in Get-Content -LiteralPath $log) {
    if ($line -match '(\d+) of (\d+) columns the same, (\d+) of (\d+) solid blocks') {
        $zones++
        $same += [long]$Matches[3]
        $total += [long]$Matches[4]
        if ($Matches[1] -eq $Matches[2]) { $exact++ }
    }
}
if ($zones -ne $files.Count) { throw "Only $zones of $($files.Count) zone summaries were recorded. See $log" }
$summary = "Audited $zones zones; $exact with all columns equal; $same of $total solid blocks equal."
[IO.File]::AppendAllText($log, "$summary`n")
Write-Output $summary
Write-Output 'Stored lighting remains included in the per-batch strict exit status; geometry counts alone do not establish parity.'
