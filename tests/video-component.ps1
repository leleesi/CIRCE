param([string]$Project,[string]$Work)
$ErrorActionPreference='Stop'
$root=Join-Path $Work 'component-test'
$fixture=Join-Path $root 'archive'
New-Item -ItemType Directory -Path "$fixture/bin" -Force | Out-Null
[IO.File]::WriteAllText("$fixture/bin/ffmpeg.exe",'test executable')
[IO.File]::WriteAllText("$fixture/bin/test.dll",'test dependency')
[IO.File]::WriteAllText("$fixture/LICENSE.txt",'test notice')
$archive=Join-Path $root 'upstream.zip'
Compress-Archive -Path "$fixture/*" -DestinationPath $archive
$asset=[pscustomobject]@{
    name='ffmpeg-master-latest-win64-lgpl-shared.zip'
    browser_download_url='https://github.com/FFmpeg-Builds/FFmpeg-Builds/releases/download/test/ffmpeg-test.zip'
    digest=('sha256:'+(Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant())
}
# Supply a local download and the Windows executable's version/configuration response.
function Invoke-RestMethod([string]$Uri) {
    if($Uri -ne 'https://api.github.com/repos/FFmpeg-Builds/FFmpeg-Builds/releases/latest'){throw 'Unexpected metadata URL'}
    return [pscustomobject]@{assets=@($asset);tag_name='test'}
}
function Invoke-WebRequest([switch]$UseBasicParsing,[string]$Uri,[string]$OutFile) {
    if($Uri -ne $asset.browser_download_url){throw 'Unexpected download URL'}
    Copy-Item -LiteralPath $archive -Destination $OutFile
}
function Start-Process([string]$FilePath,[string]$ArgumentList,[string]$RedirectStandardOutput,
                       [string]$RedirectStandardError,[switch]$NoNewWindow,[switch]$Wait,[switch]$PassThru) {
    if(!(Test-Path $FilePath) -or $ArgumentList -ne '-buildconf'){throw 'Unexpected executable'}
    [IO.File]::WriteAllText($RedirectStandardOutput,'--enable-shared')
    [IO.File]::WriteAllText($RedirectStandardError,'')
    return [pscustomobject]@{ExitCode=0}
}
foreach($lang in 'pt','en'){
    $release=Join-Path $root $lang
    New-Item -ItemType Directory -Path "$release/vendor" -Force | Out-Null
    Copy-Item -LiteralPath "$Project/tools" -Destination $release -Recurse
    [IO.File]::WriteAllText("$release/vendor/CIRCEJanela.exe",'recorder fixture')
    & "$release/tools/video.ps1" -UiLanguage $lang
    foreach($item in 'ffmpeg.exe','test.dll','avisos/FFmpeg-LICENSE.txt','avisos/FFmpeg-buildconf.txt'){
        if(!(Test-Path "$release/vendor/$item")){throw "Missing component $item"}
    }
    # Replay the download with deliberately corrupted bytes and verify the selected language.
    $badBytes=[IO.File]::ReadAllBytes($archive)
    $original=$badBytes[0]
    $badBytes[0]=$original -bxor 255
    [IO.File]::WriteAllBytes($archive,$badBytes)
    try {
        $expected=if($lang -eq 'pt'){'O pacote de vídeo recebido está incompleto. Repete a instalação.'}else{'The downloaded video package is incomplete. Run the installer again.'}
        try {& "$release/tools/video.ps1" -UiLanguage $lang; throw 'Accepted corrupt package'}
        catch {if($_.Exception.Message -cne $expected){throw}}
    }finally{
        $badBytes[0]=$original
        [IO.File]::WriteAllBytes($archive,$badBytes)
    }
}
Write-Output 'PowerShell: opção de vídeo nos dois idiomas com ZIP local, extração, publicação, hash e rejeição de pacote corrompido confirmados.'
