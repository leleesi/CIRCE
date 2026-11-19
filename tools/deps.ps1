param([switch]$Refresh,[switch]$ApenasMedicao,[switch]$ApenasVideo,[ValidateSet('en','pt')][string]$UiLanguage)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
$root=Split-Path $PSScriptRoot
. (Join-Path $PSScriptRoot 'Idioma.ps1')
$UiLanguage=Get-CirceLanguage $UiLanguage $root
$vendor=Join-Path $root 'vendor'
$notices=Join-Path $vendor 'avisos'
New-Item -Force -ItemType Directory $notices | Out-Null
if(!$ApenasVideo){
    $pm=Join-Path $vendor 'PresentMon.exe'
    $source=Join-Path $vendor 'PresentMon\PresentMon.exe'
    $expected='d74183e7ae630f72cd3690be0373ecbfdc6cbb86578148aab8fa2a7166068f34'
    if(!(Test-Path -LiteralPath $pm) -and (Test-Path -LiteralPath $source)){
        Copy-Item -LiteralPath $source -Destination $pm
        foreach($name in 'LICENSE.txt','THIRD_PARTY.txt'){
            Copy-Item -LiteralPath (Join-Path (Split-Path $source) $name) -Destination (Join-Path $notices "PresentMon-$name")
        }
    }
    if((Get-FileHash -LiteralPath $pm).Hash -ne $expected){throw (Get-CirceText 'O PresentMon está danificado. Volta a executar o instalador.' 'PresentMon is damaged. Run the installer again.')}
}
if($ApenasMedicao){return}
$cache=Join-Path $root '.cache'
New-Item -ItemType Directory -Path $cache -Force | Out-Null
$work=Join-Path $cache ([Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    $lock=Join-Path $cache 'video.json'
    if(!$Refresh -and (Test-Path -LiteralPath $lock)){
        $item=Get-Content -Raw -LiteralPath $lock | ConvertFrom-Json
    }else{
        $rel=Invoke-RestMethod 'https://api.github.com/repos/FFmpeg-Builds/FFmpeg-Builds/releases/latest'
        $asset=@($rel.assets | Where-Object name -eq 'ffmpeg-master-latest-win64-lgpl-shared.zip')[0]
        if(!$asset -or $asset.digest -notmatch '^sha256:[0-9a-f]{64}$'){throw (Get-CirceText 'O servidor não forneceu o pacote de vídeo com o respetivo hash SHA-256.' 'The server did not supply the video package with a SHA-256 digest.')}
        $item=@{sha=$asset.digest.Substring(7);url=$asset.browser_download_url;name=$asset.name;tag=$rel.tag_name}
    }
    if($item.name -ne 'ffmpeg-master-latest-win64-lgpl-shared.zip' -or $item.sha -notmatch '^[a-f0-9]{64}$' -or
       $item.url -notlike 'https://github.com/FFmpeg-Builds/FFmpeg-Builds/releases/download/*'){throw (Get-CirceText 'Metadados de vídeo inválidos.' 'Invalid video package metadata.')}
    $zip=Join-Path $work $item.name
    Invoke-WebRequest -UseBasicParsing -Uri $item.url -OutFile $zip
    if((Get-FileHash -LiteralPath $zip).Hash -ne $item.sha){throw (Get-CirceText 'O pacote de vídeo recebido está incompleto. Repete a instalação.' 'The downloaded video package is incomplete. Run the installer again.')}
    $unpack=Join-Path $work 'ffmpeg'
    Expand-Archive -LiteralPath $zip -DestinationPath $unpack
    $exe=Get-ChildItem -LiteralPath $unpack -Recurse -Filter ffmpeg.exe | Select-Object -First 1
    if(!$exe){throw (Get-CirceText 'O pacote não contém o FFmpeg.' 'The package does not contain FFmpeg.')}
    $out=Join-Path $work 'config.txt'
    $err=Join-Path $work 'erro.txt'
    $proc=Start-Process -FilePath $exe.FullName -ArgumentList '-buildconf' -RedirectStandardOutput $out -RedirectStandardError $err -NoNewWindow -Wait -PassThru
    $config=@(Get-Content -LiteralPath $out,$err) -join [Environment]::NewLine
    if($proc.ExitCode -ne 0 -or $config -notmatch '--enable-' -or $config -match '--enable-(gpl|nonfree)(\s|$)'){throw (Get-CirceText 'O FFmpeg não passou na verificação de configuração.' 'FFmpeg failed the build configuration check.')}
    $bin=$exe.Directory
    Copy-Item "$($bin.FullName)\*.dll" $vendor -Force
    Get-ChildItem -LiteralPath $bin.Parent.FullName -Recurse -File | Where-Object Name -Match '^(LICENSE|COPYING|NOTICE)' | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $notices "FFmpeg-$($_.Name)") -Force
    }
    $config | Set-Content -LiteralPath (Join-Path $notices 'FFmpeg-buildconf.txt') -Encoding UTF8
    $item | ConvertTo-Json | Set-Content -LiteralPath $lock -Encoding UTF8
    Copy-Item -LiteralPath $lock -Destination (Join-Path $notices 'FFmpeg-origem.json') -Force
    # Publish the program only after its dependencies and notices are in place.
    Copy-Item -LiteralPath $exe.FullName -Destination (Join-Path $vendor 'ffmpeg.exe') -Force
}finally{
    if(Test-Path -LiteralPath $work){Remove-Item -LiteralPath $work -Recurse -Force}
}
