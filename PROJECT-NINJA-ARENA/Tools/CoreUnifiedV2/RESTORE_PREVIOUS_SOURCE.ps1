param(
    [Parameter(Mandatory=$true)]
    [string]$ProjectRoot
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$last = Join-Path $ProjectRoot "_PNA_CORE_V2_BACKUP\.last.txt"
if (!(Test-Path $last)) { throw "No encuentro el registro del último backup." }

$backup = (Get-Content $last -Raw).Trim()
$backupSource = Join-Path $backup "Source"
$projectSource = Join-Path $ProjectRoot "Source"

if (!(Test-Path $backupSource)) { throw "Backup Source no encontrado: $backupSource" }

if (Test-Path $projectSource) { Remove-Item $projectSource -Recurse -Force }
Copy-Item $backupSource $projectSource -Recurse -Force

Write-Host "Source anterior restaurado."
