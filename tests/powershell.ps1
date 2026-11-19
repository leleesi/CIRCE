param([string]$Project=(Split-Path $PSScriptRoot),[string]$Work=[IO.Path]::GetTempPath())
$ErrorActionPreference='Stop'
$previousLocalAppData=$env:LOCALAPPDATA
$issues=@()
foreach($p in (Get-ChildItem -LiteralPath "$Project/tools","$Project/instalador" -Filter *.ps1)){
    $tokens=$null;$errors=$null
    [void][System.Management.Automation.Language.Parser]::ParseFile($p.FullName,[ref]$tokens,[ref]$errors)
    $issues+=@($errors|Where-Object {$_})
}
if($issues){throw ($issues|Out-String)}
Write-Output 'PowerShell: sintaxe dos scripts de instalação e diagnóstico verificada.'
$temporary=Join-Path $Work ([Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $temporary | Out-Null
try{
    Copy-Item -Recurse -LiteralPath "$Project/tools","$Project/vendor" -Destination $temporary
    & "$temporary/tools/deps.ps1" -ApenasMedicao
    if(!(Test-Path -LiteralPath "$temporary/vendor/PresentMon.exe")){throw 'Falta o medidor'}
    if(!(Test-Path -LiteralPath "$temporary/vendor/avisos/PresentMon-LICENSE.txt")){throw 'Falta a licença'}
    $env:LOCALAPPDATA=Join-Path $temporary 'local'
    $root=Join-Path $env:LOCALAPPDATA 'Programs/CIRCE'
    $keep=Join-Path $root ('edicao-'+[Guid]::NewGuid().ToString('N'))
    $previous=Join-Path $root ('edicao-'+[Guid]::NewGuid().ToString('N'))
    $sample=Join-Path $root 'pasta-do-utilizador'
    foreach($d in $keep,$previous,$sample){New-Item -ItemType Directory -Force $d|Out-Null}
    $oldData=Join-Path $env:LOCALAPPDATA 'Cadencia/Nativo'
    $newData=Join-Path $env:LOCALAPPDATA 'CIRCE/Nativo'
    New-Item -ItemType Directory -Force $oldData | Out-Null
    Set-Content -LiteralPath (Join-Path $oldData 'comparacao-12.cad') -Value 'primeiro'
    & "$Project/instalador/Remover.ps1" -KeepRelease $keep
    if ((Get-Content -LiteralPath (Join-Path $newData 'comparacao-12.cad')) -ne 'primeiro') { throw 'Migração falhou.' }
    New-Item -ItemType Directory -Force $oldData | Out-Null
    Set-Content -LiteralPath (Join-Path $oldData 'comparacao-12.cad') -Value 'segundo'
    & "$Project/instalador/Remover.ps1" -KeepRelease $keep
    if ((Get-Content -LiteralPath (Join-Path $newData 'comparacao-12.cad')) -ne 'primeiro' -or
        (Get-Content -LiteralPath (Join-Path $oldData 'comparacao-12.cad')) -ne 'segundo') { throw 'Conflito na migração.' }
    Write-Output 'PowerShell: migração de comparações e preservação de duas pastas existentes confirmadas.' 
    if(!(Test-Path $keep) -or (Test-Path $previous) -or !(Test-Path $sample)){throw 'Falha na manutenção de edições'}
    Write-Output 'PowerShell: instalação da medição offline e limpeza seletiva de uma edição de teste confirmadas.'
    # Run the shipped diagnostic script with deterministic PresentMon output.
    # Only the external process launch and opening Notepad are substituted.
    $originalCulture=[Globalization.CultureInfo]::CurrentCulture
    $originalUI=[Globalization.CultureInfo]::CurrentUICulture
    $originalSystemRoot=$env:SystemRoot
    $env:SystemRoot=$temporary
    try {
        . (Join-Path $Project 'tools/Idioma.ps1')
        $marker=Join-Path $temporary 'circe.lang'
        $reportRoot=Join-Path $env:LOCALAPPDATA 'CIRCE/Nativo'
        $diag=Join-Path $temporary 'tools/diagnostico.ps1'
        $linesWritten=1
        $opened=[Collections.Generic.List[string]]::new()
        function Start-Process {
            param([string]$FilePath,[object[]]$ArgumentList,[string]$RedirectStandardError,
                  [string]$RedirectStandardOutput,[string]$WindowStyle,[switch]$PassThru)
            if ($FilePath -like '*notepad.exe') { $opened.Add([string]$ArgumentList[0]); return }
            if ($FilePath -notlike '*PresentMon.exe') { throw 'Unexpected executable' }
            if ($linesWritten) {
                $csvPath=([string]$ArgumentList[-1]).Trim('"')
                @('ProcessID,SwapChainAddress,msBetweenPresents',
                  "$PID,0x1234,12.5", "$PID,0x1234,15.0", '0,0x1234,95.0') |
                    Set-Content -LiteralPath $csvPath -Encoding ASCII
            }
            $child=[pscustomobject]@{Handle=1;ExitCode=0;HasExited=$true}
            $child | Add-Member -MemberType ScriptMethod -Name WaitForExit -Value {param($ms) $true}
            return $child
        }
        foreach ($os in 'en-US','pt-PT') {
            [Globalization.CultureInfo]::CurrentCulture=[Globalization.CultureInfo]::GetCultureInfo($os)
            [Globalization.CultureInfo]::CurrentUICulture=[Globalization.CultureInfo]::GetCultureInfo($os)
            [IO.File]::WriteAllText($marker,'xx')
            if ((Get-CirceLanguage '' $temporary) -ne $os.Substring(0,2)) { throw 'System UI fallback' }
            foreach ($lang in 'pt','en') {
                $other=if ($lang -eq 'pt') {'en'} else {'pt'}
                [IO.File]::WriteAllText($marker,$other+[char]13+[char]10,[Text.Encoding]::ASCII)
                if ((Get-CirceLanguage '' $temporary) -ne $other -or
                    (Get-CirceLanguage $lang $temporary) -ne $lang) { throw 'Locale override' }
                $reportName=if ($lang -eq 'pt') {'diagnostico-fps.txt'} else {'fps-diagnostics.txt'}
                $phrase=if ($lang -eq 'pt') {'2 intervalos, média 72,73 FPS'} else {'2 intervals, average 72.73 FPS'}
                $header=if ($lang -eq 'pt') {'CIRCE · Diagnóstico de FPS'} else {'CIRCE · FPS diagnostics'}
                $frames=if ($lang -eq 'pt') {'Fotogramas do processo no CSV: 2'} else {'Frames for this process in the CSV: 2'}
                $output=Join-Path $reportRoot $reportName
                & $diag -GamePid $PID -UiLanguage $lang
                $body=Get-Content -LiteralPath $output -Raw -Encoding UTF8
                if (!$body.Contains($phrase) -or !$body.StartsWith($header) -or !$body.Contains($frames) -or
                    $opened[-1].Trim('"') -ne $output) { throw "Report $lang / $os" }
                $expected=if ($lang -eq 'pt') {'O pacote do gravador está incompleto.'} else {'The recorder package is incomplete.'}
                try {& "$temporary/tools/video.ps1" -UiLanguage $lang; throw 'No recorder error'}
                catch { if ($_.Exception.Message -cne $expected) { throw } }
                $expected=if ($lang -eq 'pt') {'Pasta de instalação desconhecida.'} else {'Unknown installation folder.'}
                try { & "$Project/instalador/Remover.ps1" -UiLanguage $lang -InstallRoot $temporary; throw 'No path error' }
                catch { if ($_.Exception.Message -cne $expected) { throw } }
            }
        }
        $linesWritten=0
        foreach ($lang in 'pt','en') {
            [IO.File]::WriteAllText($marker,$lang,[Text.Encoding]::ASCII)
            & $diag -GamePid $PID
            $expected=if ($lang -eq 'pt') {'O PresentMon não criou o ficheiro CSV.'} else {'PresentMon did not create a CSV file.'}
            if (!(Get-Content -Raw -Encoding UTF8 -LiteralPath ($opened[-1].Trim('"'))).Contains($expected)) { throw 'Missing CSV text' }
        }
        Write-Output 'PowerShell: diagnósticos PT/EN cruzados com duas regiões, dados do CSV, decimais, saída do Bloco de Notas e erros de vídeo e remoção confirmados.'
    } finally {
        [Globalization.CultureInfo]::CurrentCulture=$originalCulture
        [Globalization.CultureInfo]::CurrentUICulture=$originalUI
        $env:SystemRoot=$originalSystemRoot
    }
}finally{
    $env:LOCALAPPDATA=$previousLocalAppData
    Remove-Item -LiteralPath $temporary -Force -Recurse
}
