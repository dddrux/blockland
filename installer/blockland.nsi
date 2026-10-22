; Blockland - instalador Windows (NSIS 3, roda no Linux: apt install nsis). Gera dist/setup.exe
; makensis -DAppVersion=1.1.0.19 -DStage=../dist/stage blockland.nsi   (installer/build-installer.sh faz isso)
Unicode true
!ifndef AppVersion
  !define AppVersion "1.1.0.19"
!endif
!ifndef Stage
  !define Stage "../dist/stage"
!endif
!define APP "Blockland"
!define UNKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Blockland"
Name "${APP}"
OutFile "../dist/setup.exe"
InstallDir "$LOCALAPPDATA\Blockland"
InstallDirRegKey HKCU "Software\Blockland" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma
Icon "blockland.ico"
UninstallIcon "blockland.ico"
VIProductVersion "${AppVersion}.0"
VIAddVersionKey "ProductName" "${APP}"
VIAddVersionKey "FileVersion" "${AppVersion}"
VIAddVersionKey "FileDescription" "Instalador do ${APP}"
!include "MUI2.nsh"
!define MUI_ICON "blockland.ico"
!define MUI_UNICON "blockland.ico"
!define MUI_FINISHPAGE_RUN "$INSTDIR\blockland.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Blockland"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Spanish"
LangString KeepData ${LANG_PORTUGUESEBR} "Remover tambem seus mundos (saves) e configuracoes?$\n$\nEscolha Nao para manter."
LangString KeepData ${LANG_ENGLISH} "Also remove your worlds (saves) and settings?$\n$\nChoose No to keep them."
LangString KeepData ${LANG_SPANISH} "Eliminar tambien tus mundos (saves) y ajustes?$\n$\nElige No para conservarlos."
Section "Install"
  SetOutPath "$INSTDIR"
  ; instala ou atualiza: sobrescreve o jogo, mas NUNCA saves/logs/profile.cfg/options.cfg/gamepad.cfg nem resourcepacks existentes
  File /r /x saves /x logs /x crash-reports /x profile.cfg /x options.cfg /x gamepad.cfg /x resourcepacks "${Stage}/*.*"
  SetOutPath "$INSTDIR\assets\resourcepacks"
  SetOverwrite off
  File /nonfatal /r "${Stage}/assets/resourcepacks/*.*"
  SetOverwrite on
  SetOutPath "$INSTDIR"
  File "blockland.ico"
  WriteUninstaller "$INSTDIR\uninstall.exe"
  CreateShortcut "$DESKTOP\Blockland.lnk" "$INSTDIR\blockland.exe" "" "$INSTDIR\blockland.ico"
  CreateDirectory "$SMPROGRAMS"
  CreateShortcut "$SMPROGRAMS\Blockland.lnk" "$INSTDIR\blockland.exe" "" "$INSTDIR\blockland.ico"
  WriteRegStr HKCU "Software\Blockland" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "${UNKEY}" "DisplayName" "${APP}"
  WriteRegStr HKCU "${UNKEY}" "DisplayVersion" "${AppVersion}"
  WriteRegStr HKCU "${UNKEY}" "Publisher" "${APP}"
  WriteRegStr HKCU "${UNKEY}" "DisplayIcon" "$INSTDIR\blockland.exe"
  WriteRegStr HKCU "${UNKEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKCU "${UNKEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNKEY}" "NoRepair" 1
SectionEnd
Section "Uninstall"
  Delete "$DESKTOP\Blockland.lnk"
  Delete "$SMPROGRAMS\Blockland.lnk"
  Delete "$INSTDIR\blockland.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\blockland.ico"
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR\assets"
  MessageBox MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 "$(KeepData)" IDNO keep
    RMDir /r "$INSTDIR\saves"
    RMDir /r "$INSTDIR\logs"
    RMDir /r "$INSTDIR\crash-reports"
    Delete "$INSTDIR\profile.cfg"
    Delete "$INSTDIR\options.cfg"
    Delete "$INSTDIR\gamepad.cfg"
  keep:
  RMDir "$INSTDIR"
  DeleteRegKey HKCU "${UNKEY}"
  DeleteRegKey HKCU "Software\Blockland"
SectionEnd
