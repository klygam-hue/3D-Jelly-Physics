$ErrorActionPreference='Stop'
$toolsDir=Join-Path $PSScriptRoot 'tools'
$compilerPath=Join-Path $toolsDir 'w64devkit\bin\g++.exe'
if(Test-Path -LiteralPath $compilerPath){Write-Host 'Portable compiler already available.';exit 0}
New-Item -ItemType Directory -Force -Path $toolsDir | Out-Null
$archive=Join-Path $toolsDir 'w64devkit-x64-2.10.0.7z.exe'
Invoke-WebRequest -Uri 'https://github.com/skeeto/w64devkit/releases/download/v2.10.0/w64devkit-x64-2.10.0.7z.exe' -OutFile $archive
$actualHash=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
if($actualHash -ne '18D0A4C71A166F8401AB6305781BEC5882B40B5E06BA9807C61CB5F3B3C6325E'){throw 'Compiler archive checksum mismatch'}
$extract=Start-Process -FilePath $archive -ArgumentList @('-y',('"-o'+$toolsDir+'"')) -WindowStyle Hidden -Wait -PassThru
if($extract.ExitCode -ne 0 -or -not(Test-Path -LiteralPath $compilerPath)){throw 'Compiler extraction failed'}
Write-Host 'Portable toolchain ready. Run build_release.bat.'
