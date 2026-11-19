function Get-CirceLanguage([string]$Choice, [string]$Root) {
    if ($Choice -in 'en', 'pt') { return $Choice }
    if ($Root) {
        $marker = Join-Path $Root 'circe.lang'
        if ([IO.File]::Exists($marker)) {
            $stored = [IO.File]::ReadAllText($marker).TrimEnd([char]13,[char]10)
            if ($stored -cin 'en', 'pt') { return $stored }
        }
    }
    if ([Globalization.CultureInfo]::CurrentUICulture.TwoLetterISOLanguageName -eq 'pt') { return 'pt' }
    return 'en'
}
# The entry point sets $UiLanguage once, including for work performed in child scopes.
function Get-CirceText([string]$Portuguese, [string]$English) {
    if ($UiLanguage -eq 'pt') { return $Portuguese }
    return $English
}
