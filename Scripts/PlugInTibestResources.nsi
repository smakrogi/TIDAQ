; PlugInTibestResources.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "PlugInTibestResources"

; The file to write
OutFile "PlugInTibestResources.exe"

; The default installation directory
InstallDir $PROGRAMFILES\mipav\

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_PlugInTibestResources" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

; Text in license.
LicenseText ""
LicenseData lgpl-2.1.txt
;--------------------------------

; Pages
Page license
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "PlugInTibestResources (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File ITKCommon-4.1.dll
  
  SetOutPath $INSTDIR\plugins\images
  File *.png
  
  SetOutPath $INSTDIR\plugins\libs
  File ITKCommon-4.1.dll PQCT_Analysis.dll 
  File PQCT_Analysis_Params.txt PQCT_Analysis_Params_4PCT.txt PQCT_Analysis_Params_66PCT.txt CT_Analysis_Params_MidThigh.txt
  File PQCT_AnalysisITK.exe
  File lgpl-2.1.txt
  
  SetOutPath $INSTDIR\results
  File Readme.txt
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_PlugInTibestResources "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PlugInTibestResources" "DisplayName" "NSIS Example2"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PlugInTibestResources" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PlugInTibestResources" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PlugInTibestResources" "NoRepair" 1
  WriteUninstaller "PlugInTibestResources_Uninstall.exe"
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PlugInTibestResources"
  DeleteRegKey HKLM SOFTWARE\NSIS_PlugInTibestResources

  ; Remove files and uninstaller
  Delete $INSTDIR\ITKCommon-4.1.dll
  Delete $INSTDIR\plugins\images\*.png
  Delete $INSTDIR\plugins\libs\*.txt
  Delete $INSTDIR\plugins\libs\ITKCommon-4.1.dll
  Delete $INSTDIR\plugins\libs\PQCT_Analysis.dll
  Delete $INSTDIR\plugins\libs\PQCT_AnalysisITK.exe
  Delete $INSTDIR\plugins\libs\PQCT_Analysis_Params.txt
  Delete $INSTDIR\plugins\libs\PQCT_Analysis_Params_4PCT.txt
  Delete $INSTDIR\plugins\libs\PQCT_Analysis_Params_66PCT.txt
  Delete $INSTDIR\plugins\libs\CT_Analysis_Params_MidThigh.txt
  Delete $INSTDIR\plugins\libs\lgpl-2.1.txt
  Delete $INSTDIR\results\Readme.txt
  Delete $INSTDIR\PlugInTibestResources_Uninstall.exe
  
  ; Remove directories used
  RMDir $INSTDIR\plugins\images
  RMDir $INSTDIR\plugins\libs	
  RMDir $INSTDIR\plugins\results
  
SectionEnd
