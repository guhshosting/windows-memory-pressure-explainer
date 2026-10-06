param([Parameter(Mandatory=$true)][string]$Fixture)
$ErrorActionPreference='Stop'
$raw=(& $Fixture | Out-String)
if($LASTEXITCODE -ne 0) {throw 'fixture failed'}
$j=ConvertFrom-Json $raw
if($j.schema_version -ne 1 -or $j.application_version -ne '1.0.0' -or $j.status -ne 'partial') {throw 'root schema'}
if($j.system.compression_bytes -ne $null -or $j.system.physical_available_bytes -ne 0 -or $j.system.commit_headroom_bytes -ne 10) {throw 'null and zero semantics'}
if($j.system.commit_utilization_percent -ne 90 -or $j.system.physical_utilization_percent -ne 100) {throw 'derived metrics'}
if($j.processes.Count -ne 3 -or $j.processes[0].pid -ne 2 -or $j.processes[1].pid -ne 9) {throw 'ordering'}
if($j.processes[0].working_set_bytes -ne 12 -or $j.processes[0].private_usage_bytes -ne 34) {throw 'working set/private distinction'}
if($j.processes[0].name -notlike '*"\*' -or -not $j.processes[0].name.Contains([char]10)) {throw 'string roundtrip'}
if($j.processes[2].working_set_bytes -ne $null -or $j.processes[2].win32_error -ne 5) {throw 'unavailable process'}
if($j.warnings[0].count -ne 1 -or $j.findings[0].code -ne 'commit_near_limit') {throw 'structured warnings/findings'}
$again=(& $Fixture | Out-String)
if($raw -cne $again) {throw 'determinism'}
'Independent JSON fixture checks passed'
