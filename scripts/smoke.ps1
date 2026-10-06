param([Parameter(Mandatory=$true)][string]$Executable,[string]$OutputDirectory='artifacts/live')
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
foreach($command in @('summary','processes','explain')) {
 $raw=(& $Executable $command --top 3 --json | Out-String)
 $code=$LASTEXITCODE
 if($code -notin @(0,1)) {throw "$command failed: $code"}
 $raw | Set-Content -Encoding utf8 "$OutputDirectory/$command.json"
 $j=ConvertFrom-Json $raw
 if($j.schema_version -ne 1 -or $j.system.page_size_bytes -le 0 -or $j.system.physical_total_bytes -le 0) {throw 'system metrics'}
 if($j.processes.Count -gt 3) {throw 'top bound'}
 if($command -eq 'summary' -and ($j.processes_requested -or $j.processes.Count -ne 0)) {throw 'summary scope'}
 foreach($row in $j.processes) {if($null -ne $row.working_set_bytes -and $row.working_set_bytes -is [string]) {throw 'numeric bytes'}}
 "$command exit=$code status=$($j.status) rows=$($j.processes.Count)" | Add-Content "$OutputDirectory/results.txt"
}
& $Executable --version | Out-File "$OutputDirectory/version.txt"
if($LASTEXITCODE -ne 0) {throw 'version'}
& $Executable --help | Out-File "$OutputDirectory/help.txt"
if($LASTEXITCODE -ne 0) {throw 'help'}
& $Executable --top 101 2>&1 | Out-File "$OutputDirectory/invalid.txt"
if($LASTEXITCODE -ne 2) {throw 'invalid usage exit'}
'Smoke checks passed'
