param([ValidateSet('en','pt')][string]$UiLanguage)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Idioma.ps1')
$root=Split-Path $PSScriptRoot
$UiLanguage=Get-CirceLanguage $UiLanguage $root
if (!(Test-Path -LiteralPath (Join-Path $root 'vendor/CIRCEJanela.exe'))) {
    throw (Get-CirceText 'O pacote do gravador está incompleto.' 'The recorder package is incomplete.')
}
& (Join-Path $PSScriptRoot 'deps.ps1') -ApenasVideo -UiLanguage $UiLanguage
