!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

Name "UltraIDE"
OutFile "UltraIDE_Setup.exe"
InstallDir "$PROGRAMFILES\UltraIDE"
RequestExecutionLevel admin

Page directory
Page instfiles

Section "MainSection" SEC01
    SetOutPath "$INSTDIR"
    
    File "UltraIDE.exe"
    File "config.json"
    
    WriteRegStr HKCR "*\shell\UltraIDE" "" "Открыть в UltraIDE"
    WriteRegStr HKCR "*\shell\UltraIDE\command" "" '"$INSTDIR\UltraIDE.exe" "%1"'
    
    WriteRegStr HKCR "Directory\shell\UltraIDE" "" "Открыть папку в UltraIDE"
    WriteRegStr HKCR "Directory\shell\UltraIDE\command" "" '"$INSTDIR\UltraIDE.exe" "%V"'
    
    CreateShortCut "$DESKTOP\UltraIDE.lnk" "$INSTDIR\UltraIDE.exe"
    
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
    DeleteRegKey HKCR "*\shell\UltraIDE"
    DeleteRegKey HKCR "Directory\shell\UltraIDE"
    Delete "$INSTDIR\UltraIDE.exe"
    Delete "$DESKTOP\UltraIDE.lnk"
    RMDir "$INSTDIR"
SectionEnd