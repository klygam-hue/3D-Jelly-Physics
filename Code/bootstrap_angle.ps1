param()
$ErrorActionPreference='Stop'
$angleRoot=Join-Path $PSScriptRoot 'vendor\angle'
$archiveRoot=Join-Path $PSScriptRoot 'tools\downloads'
New-Item -ItemType Directory -Force -Path $angleRoot,$archiveRoot | Out-Null
$archivePath=Join-Path $archiveRoot 'electron-v41.0.0-win32-x64.zip'
$expectedArchive='2E69A07219B05B625B3A2631A3E025F2F7B9DDB7419C49F1A5623D04C0D74D91'
if(!(Test-Path -LiteralPath $archivePath)){
    Invoke-WebRequest -Uri 'https://github.com/electron/electron/releases/download/v41.0.0/electron-v41.0.0-win32-x64.zip' -OutFile $archivePath
}
if((Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash -ne $expectedArchive){throw 'ANGLE archive SHA256 mismatch; nothing was extracted.'}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[System.IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    foreach($name in 'libEGL.dll','libGLESv2.dll','vulkan-1.dll','LICENSES.chromium.html'){
        $entry=$archive.GetEntry($name)
        if(!$entry){throw "Pinned runtime entry missing: $name"}
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry,(Join-Path $angleRoot $name),$true)
    }
}finally{$archive.Dispose()}
Write-Host 'Pinned ANGLE runtime and notices restored. No browser is installed or required.'
