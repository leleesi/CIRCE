Unicode true
Target amd64-unicode
!addplugindir /amd64-unicode "../bin"
RequestExecutionLevel user
!include "MUI2.nsh"
!include "x64.nsh"
!include "Sections.nsh"
!include "StrFunc.nsh"
${Using:StrFunc} StrRep

Name "CIRCE"
OutFile "../CIRCE-Setup.exe"
InstallDir "$LOCALAPPDATA\Programs\CIRCE"
BrandingText "CIRCE"
VIProductVersion "0.0.1.0"
VIFileVersion "0.0.1.4"
VIAddVersionKey /LANG=2070 "ProductName" "CIRCE"
VIAddVersionKey /LANG=2070 "ProductVersion" "0.0.1"
VIAddVersionKey /LANG=2070 "FileVersion" "0.0.1.4"
VIAddVersionKey /LANG=2070 "FileDescription" "Instalador do CIRCE"
VIAddVersionKey /LANG=2070 "LegalCopyright" "Copyright (c) 2026 leleesi and CIRCE contributors"
VIAddVersionKey /LANG=1033 "ProductName" "CIRCE"
VIAddVersionKey /LANG=1033 "ProductVersion" "0.0.1"
VIAddVersionKey /LANG=1033 "FileVersion" "0.0.1.4"
VIAddVersionKey /LANG=1033 "FileDescription" "CIRCE installer"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright (c) 2026 leleesi and CIRCE contributors"
SetCompressor /SOLID lzma
!define productKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\CIRCE"
!define ps '"$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -NonInteractive'
!define previousKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\CadenciaNativo"
Var legacyUpgrade
Var release
Var result
Var details
Var uiTag

!define MUI_LANGDLL_REGISTRY_ROOT HKCU
!define MUI_LANGDLL_REGISTRY_KEY "${productKey}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "InstallerLanguage"
!define MUI_LANGDLL_ALWAYSSHOW
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_LANGDLL_WINDOWTITLE "CIRCE · Idioma / Language"
!define MUI_LANGDLL_INFO "Escolhe o idioma do CIRCE. / Choose your CIRCE language."
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "../assets/installer-header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "../assets/installer-header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "../assets/installer.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "../assets/installer.bmp"
!define MUI_ICON "../assets/marca.ico"
!define MUI_UNICON "../assets/marca.ico"
!define MUI_WELCOMEPAGE_TEXT "$(CirceWelcome)"
!define MUI_CUSTOMFUNCTION_GUIINIT InitShortcut
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchApp
!define MUI_FINISHPAGE_RUN_TEXT "$(CirceLaunch)"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Portuguese"
!include "idiomas.nsh"
ReserveFile /plugin CIRCEIdioma.dll
!insertmacro MUI_RESERVEFILE_LANGDLL

; Keep the NSIS language table and saved choice in sync before any page opens.
!macro readSavedLanguage
  ReadRegStr $0 HKCU "${productKey}" "InstallerLanguage"
  ${If} $0 == ${LANG_PORTUGUESE}
  ${OrIf} $0 == ${LANG_ENGLISH}
    StrCpy $LANGUAGE $0
  ${EndIf}
!macroend

!macro showLanguage
  ${Unless} ${Silent}
    ClearErrors
    ; A failed DLL load can leave its input on the stack. Only an explicit selection returns a bare ID.
    CIRCEIdioma::Choose "preferred-$LANGUAGE"
    Pop $0
    ${If} $0 == "cancel"
      Abort
    ${ElseIf} $0 == ${LANG_PORTUGUESE}
    ${OrIf} $0 == ${LANG_ENGLISH}
      StrCpy $LANGUAGE $0
    ${Else}
      ; Preserve interactive installation even if the custom dialog cannot be created.
      !insertmacro MUI_LANGDLL_DISPLAY
      ${If} $LANGUAGE != ${LANG_PORTUGUESE}
      ${AndIf} $LANGUAGE != ${LANG_ENGLISH}
        MessageBox MB_ICONSTOP "Não foi possível abrir a escolha de idioma. / The language selection could not be opened."
        Abort
      ${EndIf}
    ${EndIf}
  ${EndUnless}
!macroend

!macro selectTag
  ${If} $LANGUAGE == ${LANG_PORTUGUESE}
    StrCpy $uiTag "pt"
  ${Else}
    StrCpy $uiTag "en"
  ${EndIf}
!macroend

Function LaunchApp
  ExecShell "open" "$release\CIRCE.exe"
FunctionEnd

!macro lockWizard
  System::Call 'kernel32::CreateMutexW(p 0, i 0, w "Local\CIRCE.Setup.Instance") p .r1 ?e'
  Pop $0
  ${If} $0 = 183
    MessageBox MB_ICONEXCLAMATION "$(CirceBusy)"
    Abort
  ${EndIf}
!macroend

!macro closeRunning
  retry:
  FindWindow $0 "CIRCE"
  ${If} $0 == 0
    FindWindow $0 "CadenciaNativo"
  ${EndIf}
  ${If} $0 != 0
    MessageBox MB_RETRYCANCEL|MB_ICONINFORMATION "$(CirceCloseApp)" IDRETRY retry
    Abort
  ${EndIf}
!macroend

Function un.onInit
  !insertmacro readSavedLanguage
  ${If} $0 != ${LANG_PORTUGUESE}
  ${AndIf} $0 != ${LANG_ENGLISH}
    !insertmacro showLanguage
  ${EndIf}
  !insertmacro selectTag
  !insertmacro lockWizard
  !insertmacro closeRunning
FunctionEnd

Function .onInit
  !insertmacro readSavedLanguage
  !insertmacro showLanguage
  !insertmacro selectTag
  !insertmacro lockWizard
  !insertmacro closeRunning
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentBuildNumber"
  ${IfNot} ${IsNativeAMD64}
  ${OrIf} $0 < 22000
    MessageBox MB_ICONSTOP "$(CirceWindows)"
    Abort
  ${EndIf}
  StrCpy $legacyUpgrade 0
  ReadRegStr $0 HKCU "${productKey}" "InstallLocation"
  ${If} $0 == "$LOCALAPPDATA\Programs\CadenciaNativo"
    StrCpy $INSTDIR "$LOCALAPPDATA\Programs\CadenciaNativo"
  ${ElseIf} $0 == ""
    ReadRegStr $0 HKCU "${previousKey}" "InstallLocation"
    ${If} $0 == "$LOCALAPPDATA\Programs\CadenciaNativo"
      StrCpy $INSTDIR "$LOCALAPPDATA\Programs\CadenciaNativo"
      StrCpy $legacyUpgrade 1
    ${EndIf}
  ${EndIf}
FunctionEnd

Section "$(CirceCore)" core
  SectionIn RO
  DetailPrint "$(CircePreparing)"
  System::Call 'ole32::CoCreateGuid(g .r1) i .r0'
  StrCpy $result $0
  StrCpy $1 $1 36 1
  ${StrRep} $details "$1" "-" ""
  StrLen $0 $details
  ${If} $result != 0
  ${OrIf} $0 != 32
    Abort "$(CirceCreateFolder)"
  ${EndIf}
  StrCpy $release "$INSTDIR\edicao-$details"
  SetOutPath "$release"
  File "../bin/CIRCE.exe"
  File "../LICENSE"
  ClearErrors
  FileOpen $1 "$release\circe.lang" w
  IfErrors languageFailed
  FileWrite $1 "$uiTag$\r$\n"
  FileClose $1
  IfErrors languageFailed
  Goto languageReady
  languageFailed:
    SetOutPath "$TEMP"
    RMDir /r "$release"
    Abort "$(CirceLanguageError)"
  languageReady:
  SetOutPath "$release\assets"
  File "../assets/marca.ico"
  SetOutPath "$release\vendor"
  File "../bin/CIRCEJanela.exe"
  SetOutPath "$release\tools"
  File "../tools/*.ps1"
  DetailPrint "$(CirceMeasure)"
  SetOutPath "$release\vendor"
  File "../vendor/PresentMon/PresentMon.exe"
  SetOutPath "$release\vendor\avisos"
  File "/oname=PresentMon-LICENSE.txt" "../vendor/PresentMon/LICENSE.txt"
  File "/oname=PresentMon-THIRD_PARTY.txt" "../vendor/PresentMon/THIRD_PARTY.txt"
SectionEnd

Section "$(CirceVideo)" video
  DetailPrint "$(CirceDownloading)"
  nsExec::ExecToStack '${ps} -ExecutionPolicy RemoteSigned -File "$release\tools\video.ps1" -UiLanguage "$uiTag"'
  Pop $result
  Pop $details
  ${If} $result != 0
    DetailPrint "$details"
    MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(CirceVideoError)" IDYES videoDone
    SetOutPath "$TEMP"
    RMDir /r "$release"
    Abort "$(CirceStopped)"
  ${EndIf}
  videoDone:
SectionEnd

Section /o "$(CirceShortcut)" shortcut
SectionEnd

Section "-Concluir"
  InitPluginsDir
  ClearErrors
  SetOutPath "$release"
  WriteUninstaller "$INSTDIR\Desinstalar.exe"
  CreateShortcut "$SMPROGRAMS\CIRCE.lnk" "$release\CIRCE.exe" "" "$release\assets\marca.ico"
  ${If} ${SectionIsSelected} ${shortcut}
    CreateShortcut "$DESKTOP\CIRCE.lnk" "$release\CIRCE.exe" "" "$release\assets\marca.ico"
  ${Else}
    IfFileExists "$DESKTOP\CIRCE.lnk" 0 +2
      Delete "$DESKTOP\CIRCE.lnk"
  ${EndIf}
  WriteRegStr HKCU "${productKey}" "DisplayName" "CIRCE"
  WriteRegStr HKCU "${productKey}" "InstallerLanguage" $LANGUAGE
  WriteRegStr HKCU "${productKey}" "DisplayVersion" "0.0.1"
  WriteRegStr HKCU "${productKey}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${productKey}" "DisplayIcon" "$release\assets\marca.ico"
  WriteRegStr HKCU "${productKey}" "UninstallString" '"$INSTDIR\Desinstalar.exe"'
  WriteRegDWORD HKCU "${productKey}" "NoModify" 1
  WriteRegDWORD HKCU "${productKey}" "NoRepair" 1
  IfErrors publishFailed
  ; Refresh only the links just published, including when updating an existing installation.
  System::Call 'shell32::SHChangeNotify(i 0x2000, i 5, w "$SMPROGRAMS\CIRCE.lnk", p 0)'
  ${If} ${SectionIsSelected} ${shortcut}
    System::Call 'shell32::SHChangeNotify(i 0x2000, i 5, w "$DESKTOP\CIRCE.lnk", p 0)'
  ${EndIf}
  SetOutPath "$PLUGINSDIR"
  File "Remover.ps1"
  File "../tools/Idioma.ps1"
  nsExec::ExecToLog '${ps} -ExecutionPolicy RemoteSigned -File "$PLUGINSDIR\Remover.ps1" -UiLanguage "$uiTag" -InstallRoot "$INSTDIR" -KeepRelease "$release"'
  Pop $result
  ${If} $result != 0
    DetailPrint "$(CirceMaintenance)"
  ${EndIf}
  ${If} $legacyUpgrade == 1
    DeleteRegKey HKCU "${previousKey}"
  ${EndIf}
  Goto published
  publishFailed:
    Abort "$(CircePublishError)"
  published:

SectionEnd

Function InitShortcut
  IfFileExists "$DESKTOP\CIRCE.lnk" selectShortcut 0
  StrCmp $legacyUpgrade 1 0 done
  IfFileExists "$DESKTOP\Cadência C++.lnk" selectShortcut done
  selectShortcut:
    !insertmacro SelectSection ${shortcut}
  done:
FunctionEnd

Section "Uninstall"
  InitPluginsDir
  SetOutPath "$PLUGINSDIR"
  File "Remover.ps1"
  File "../tools/Idioma.ps1"
  DetailPrint "$(CirceRemoving)"
  nsExec::ExecToStack '${ps} -ExecutionPolicy RemoteSigned -File "$PLUGINSDIR\Remover.ps1" -UiLanguage "$uiTag" -InstallRoot "$INSTDIR"'
  Pop $result
  Pop $details
  ${If} $result != 0
    DetailPrint "$details"
    Abort "$(CirceUninstallError)"
  ${EndIf}
  DeleteRegKey HKCU "${productKey}"
  Delete "$INSTDIR\Remover.ps1"
  Delete "$INSTDIR\Desinstalar.exe"
  RMDir "$INSTDIR"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${core} "$(CirceCoreHelp)"
  !insertmacro MUI_DESCRIPTION_TEXT ${video} "$(CirceVideoHelp)"
  !insertmacro MUI_DESCRIPTION_TEXT ${shortcut} "$(CirceShortcutHelp)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END
