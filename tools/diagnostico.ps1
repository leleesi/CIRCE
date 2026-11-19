param([Parameter(Mandatory=$true)][int]$GamePid,[ValidateSet('en','pt')][string]$UiLanguage)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Idioma.ps1')
$root=Split-Path $PSScriptRoot
$UiLanguage=Get-CirceLanguage $UiLanguage $root
$culture=[Globalization.CultureInfo]::GetCultureInfo((Get-CirceText 'pt-PT' 'en-US'))
$exe=Join-Path $root 'vendor\PresentMon.exe'
$folder=Join-Path $env:LOCALAPPDATA 'CIRCE\Nativo'
$tmp=Join-Path $folder ('ensaio-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tmp | Out-Null
$reportName=Get-CirceText 'diagnostico-fps.txt' 'fps-diagnostics.txt'
$report=Join-Path $folder $reportName
$lines=@(
    (Get-CirceText 'CIRCE · Diagnóstico de FPS' 'CIRCE · FPS diagnostics')
    ((Get-CirceText 'Data: ' 'Date: ')+(Get-Date -Format o))
    ((Get-CirceText 'Processo selecionado: ' 'Selected process: ')+$GamePid)
)
$proc=$null
$name='CIRCEDiag'+[Guid]::NewGuid().ToString('N')
try {
    $game=Get-Process -Id $GamePid
    $lines+=(Get-CirceText 'Executável: ' 'Executable: ')+$game.ProcessName
    $lines+="PresentMon: $((Get-Item $exe).VersionInfo.FileVersion)"
    $csv=Join-Path $tmp 'events.csv'
    $argv=@('--process_id',$GamePid,'--v1_metrics','--qpc_time','--no_track_gpu','--no_track_input','--timed',8,'--terminate_after_timed','--no_console_stats','--session_name',$name,'--output_file',('"' + $csv + '"'))
    $proc=Start-Process -FilePath $exe -ArgumentList $argv -RedirectStandardError "$tmp/error.txt" -RedirectStandardOutput "$tmp/out.txt" -WindowStyle Hidden -PassThru
    $null=$proc.Handle
    if(!$proc.WaitForExit(20000)){throw (Get-CirceText 'O ensaio excedeu 20 segundos.' 'The test timed out after 20 seconds.')}
    $lines+=(Get-CirceText 'Código de saída do ensaio independente: ' 'Independent test exit code: ')+$proc.ExitCode
    if(Test-Path $csv){
        $lines+=(Get-CirceText 'Cabeçalho: ' 'Header: ')+(Get-Content -LiteralPath $csv -TotalCount 1)
        $rows=@(Import-Csv -LiteralPath $csv | Where-Object ProcessID -eq $GamePid)
        $lines+=(Get-CirceText 'Fotogramas do processo no CSV: ' 'Frames for this process in the CSV: ')+$rows.Count
        $groups=$rows | Group-Object SwapChainAddress | Sort-Object Count -Descending
        foreach($g in ($groups | Select-Object -First 3)){
            $sum=0.0;$n=0
            foreach($r in $g.Group){
                $ms=0.0
                if([double]::TryParse($r.msBetweenPresents,[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$ms) -and $ms -gt 0){$sum+=$ms;$n++}
            }
            if($n){
                $rate=(1000*$n/$sum).ToString('0.##',$culture)
                $lines+=(Get-CirceText 'Superfície {0}: {1} intervalos, média {2} FPS' 'Swap chain {0}: {1} intervals, average {2} FPS') -f $g.Name,$n,$rate
            }
        }
    } else {$lines+=Get-CirceText 'O PresentMon não criou o ficheiro CSV.' 'PresentMon did not create a CSV file.'}
} catch {$lines+=(Get-CirceText 'Erro: ' 'Error: ')+$_.Exception.Message}
finally {
    if($proc -and !$proc.HasExited){$proc.Kill();$proc.WaitForExit(); & "$env:SystemRoot\System32\logman.exe" stop $name -ets | Out-Null}
    foreach($p in @("$tmp/error.txt","$tmp/out.txt","$folder/presentmon.log","$folder/leitor-fps.txt","$folder/video.log")){
        if(Test-Path $p){$lines+=(Split-Path $p -Leaf);$lines+=@(Get-Content -LiteralPath $p -TotalCount 18)}
    }
    $draft=Join-Path $tmp $reportName
    $lines | Set-Content -Encoding UTF8 -LiteralPath $draft
    Move-Item -LiteralPath $draft -Destination $report -Force
    Remove-Item -LiteralPath $tmp -Recurse -Force
    Start-Process -FilePath "$env:SystemRoot\System32\notepad.exe" -ArgumentList ('"'+$report+'"')
}
