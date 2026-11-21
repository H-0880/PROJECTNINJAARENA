param(
    [Parameter(Mandatory=$true)]
    [string]$ProjectRoot
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$PackageRoot = Split-Path -Parent $PSScriptRoot
$SourceCandidate = Join-Path $PackageRoot "Source"
$ProjectSource = Join-Path $ProjectRoot "Source"

if (!(Test-Path $SourceCandidate)) { throw "No encuentro Source dentro del paquete." }
if (!(Test-Path $ProjectSource)) { throw "No encuentro Source dentro del proyecto." }

$uprojects = @(Get-ChildItem $ProjectRoot -Filter *.uproject -File)
if ($uprojects.Count -ne 1) { throw "Se esperaba exactamente un .uproject en ProjectRoot." }

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$backup = Join-Path $ProjectRoot "_PNA_CORE_V2_BACKUP\$stamp"
New-Item -ItemType Directory -Force -Path $backup | Out-Null
Copy-Item $ProjectSource (Join-Path $backup "Source") -Recurse -Force
Set-Content (Join-Path $ProjectRoot "_PNA_CORE_V2_BACKUP\.last.txt") $backup -Encoding UTF8

Copy-Item (Join-Path $SourceCandidate "*") $ProjectSource -Recurse -Force

Write-Host "Core Unified V2 aplicado."
Write-Host "Backup: $backup"
Write-Host "Content, Config y mapas no fueron modificados."
