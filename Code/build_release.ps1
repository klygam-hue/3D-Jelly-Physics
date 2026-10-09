param([ValidateSet('Release','Debug')][string]$Configuration='Release',[switch]$SkipTests)
$ErrorActionPreference='Stop'
$toolBin=Join-Path $PSScriptRoot 'tools\w64devkit\bin'
if(Test-Path -LiteralPath (Join-Path $toolBin 'g++.exe')) {
    $env:PATH=$toolBin+';'+$env:PATH
    $cmakeExe=Join-Path $toolBin 'cmake.exe'
    $ctestExe=Join-Path $toolBin 'ctest.exe'
    $buildDir=Join-Path $PSScriptRoot ('build\local-'+$Configuration.ToLowerInvariant())
    & $cmakeExe -S $PSScriptRoot -B $buildDir -G 'MinGW Makefiles' "-DCMAKE_BUILD_TYPE=$Configuration" '-Wno-deprecated'
} else {
    $cmakeCommand=Get-Command cmake -ErrorAction SilentlyContinue
    if(-not $cmakeCommand) {throw 'CMake/compiler missing. Run bootstrap_tools.ps1 or install Visual Studio C++ with CMake.'}
    $cmakeExe=$cmakeCommand.Source
    $ctestExe=Join-Path (Split-Path $cmakeExe) 'ctest.exe'
    $buildDir=Join-Path $PSScriptRoot 'build\vs2022'
    & $cmakeExe -S $PSScriptRoot -B $buildDir -G 'Visual Studio 17 2022' -A x64 '-Wno-deprecated'
}
if($LASTEXITCODE -ne 0){throw 'CMake configuration failed'}
& $cmakeExe --build $buildDir --config $Configuration --parallel
if($LASTEXITCODE -ne 0){throw 'Compilation failed'}
if(-not $SkipTests) {
    & $ctestExe --test-dir $buildDir -C $Configuration --output-on-failure
    if($LASTEXITCODE -ne 0){throw 'Physics regression failed; release was not installed'}
}
if($Configuration -eq 'Release') {
    & $cmakeExe --install $buildDir --config $Configuration --prefix $PSScriptRoot
    if($LASTEXITCODE -ne 0){throw 'Release installation failed'}
    Write-Host "Built: $PSScriptRoot\Release\3D_Jelly_Physics.exe"
} else {
    Write-Host "Built: $buildDir\Debug\3D_Jelly_Physics.exe"
}
