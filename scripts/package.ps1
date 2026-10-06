param([string]$BuildDirectory='build')
$ErrorActionPreference='Stop'
$repoRoot=(Resolve-Path "$PSScriptRoot/..").Path
Set-Location $repoRoot
$dist=Join-Path $repoRoot 'dist'
New-Item -ItemType Directory -Force $dist | Out-Null
$stage=Join-Path $dist 'windows-memory-pressure-explainer-1.0.0-windows-x64'
if(Test-Path $stage) {throw 'Staging directory already exists; use a fresh workspace or explicitly remove the old stage after reviewing it.'}
New-Item -ItemType Directory $stage | Out-Null
Copy-Item "$BuildDirectory/Release/windows-memory-pressure-explainer.exe" $stage
Copy-Item README.md,LICENSE $stage
Copy-Item docs "$stage/docs" -Recurse
$zip=Join-Path $dist 'windows-memory-pressure-explainer-1.0.0-windows-x64.zip'
if(Test-Path $zip) {throw 'ZIP already exists'}
Compress-Archive -LiteralPath $stage -DestinationPath $zip -CompressionLevel Optimal
$hash=(Get-FileHash -Algorithm SHA256 $zip).Hash.ToLowerInvariant()
"$hash  $([System.IO.Path]::GetFileName($zip))" | Set-Content -Encoding ascii "$zip.sha256"
Write-Output $zip
Write-Output "$zip.sha256"
