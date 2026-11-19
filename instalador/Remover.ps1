param([string]$KeepRelease,[string]$InstallRoot=(Join-Path $env:LOCALAPPDATA 'Programs/CIRCE'),[ValidateSet('en','pt')][string]$UiLanguage)
$ErrorActionPreference='Stop'
$helper=Join-Path $PSScriptRoot 'Idioma.ps1'
if (!(Test-Path -LiteralPath $helper)) { $helper=Join-Path (Split-Path $PSScriptRoot) 'tools/Idioma.ps1' }
. $helper
$UiLanguage=Get-CirceLanguage $UiLanguage $KeepRelease
$root=[IO.Path]::GetFullPath($InstallRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)
$expected=[IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'Programs/CIRCE'))
$previousRoot=[IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'Programs/CadenciaNativo'))
if ($root -ne $expected -and $root -ne $previousRoot) { throw (Get-CirceText 'Pasta de instalação desconhecida.' 'Unknown installation folder.') }
$editions=@(Get-ChildItem -LiteralPath $root -Directory | Where-Object {
    $_.Name -match '^(edicao-|versao-)[0-9a-f]{32}$' -and $_.FullName -ne $KeepRelease -and
    !(($_.Attributes -band [IO.FileAttributes]::ReparsePoint))
})
# Check for a locked executable before removing any of the files of that edition.
$removable=@()
foreach($dir in $editions){
    $locked=$false
    foreach($exe in @('CIRCE.exe','Cadencia.exe','vendor/PresentMon.exe','vendor/CIRCEJanela.exe','vendor/CadenciaJanela.exe','vendor/ffmpeg.exe')){
        $path=Join-Path $dir.FullName $exe
        if([IO.File]::Exists($path)){
            try {$probe=[IO.File]::Open($path,'Open','ReadWrite','None');$probe.Dispose()}
            catch {$locked=$true;break}
        }
    }
    if($locked){
        if($KeepRelease){continue}
        throw (Get-CirceText 'A edição está em utilização. Fecha o CIRCE e tenta novamente.' 'This edition is in use. Close CIRCE and try again.')
    }
    $removable+=$dir
}
foreach($dir in $removable){
    try {Remove-Item -LiteralPath $dir.FullName -Recurse -Force}
    catch {if(!$KeepRelease){throw}}
}
if ($KeepRelease) {
    $oldData=Join-Path $env:LOCALAPPDATA 'Cadencia/Nativo'
    $newBase=Join-Path $env:LOCALAPPDATA 'CIRCE'
    $newData=Join-Path $newBase 'Nativo'
    if ((Test-Path -LiteralPath $oldData -PathType Container) -and !(Test-Path -LiteralPath $newData)) {
        New-Item -ItemType Directory -Path $newBase -Force | Out-Null
        [IO.Directory]::Move($oldData,$newData)
    }
    if ($root -ne $previousRoot) { return }
}
$shell=New-Object -ComObject WScript.Shell
foreach($special in 'DesktopDirectory','Programs'){
    $names=if ($KeepRelease) { @('Cadência C++.lnk') } else { @('CIRCE.lnk','Cadência C++.lnk') }
    foreach($name in $names) {
        $shortcut=Join-Path ([Environment]::GetFolderPath($special)) $name
        if ((Test-Path -LiteralPath $shortcut) -and
            $shell.CreateShortcut($shortcut).TargetPath.StartsWith(($root+'\'),[StringComparison]::OrdinalIgnoreCase)) {
            Remove-Item -LiteralPath $shortcut -Force
        }
    }
}
