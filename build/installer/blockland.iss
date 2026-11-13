; Blockland - instalador Windows (Inno Setup 6). Gera o setup.exe
; Compilar: ISCC.exe /DAppVersion=1.1.0.19 /DStage=..\work\stage blockland.iss   (installer\build-installer.sh faz isso)
#ifndef AppVersion
  #define AppVersion "1.1.0.19"
#endif
#ifndef Stage
  #define Stage "..\work\stage"
#endif
#ifndef OutDir
  #define OutDir "..\..\executaveis"
#endif
#ifndef OutName
  #define OutName "setup"
#endif
[Setup]
AppId={{B7C1E5D2-4A0F-4D8E-9C63-5E2B1F7A9D40}
AppName=Blockland
AppVersion={#AppVersion}
AppPublisher=Blockland
VersionInfoVersion={#AppVersion}
DefaultDirName={localappdata}\Blockland
UsePreviousAppDir=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
DisableDirPage=auto
PrivilegesRequired=lowest
#ifndef Arch32
ArchitecturesAllowed=x64
#endif
SetupIconFile=blockland.ico
UninstallDisplayIcon={app}\blockland.exe
UninstallDisplayName=Blockland
OutputDir={#OutDir}
OutputBaseFilename={#OutName}
Compression=lzma2
SolidCompression=no
WizardStyle=modern
ShowLanguageDialog=no
LanguageDetectionMethod=uilanguage
CloseApplications=yes
RestartApplications=no
[Languages]
Name: "ptbr"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
[CustomMessages]
ptbr.KeepData=Remover tambem seus mundos (saves) e configuracoes?%n%nEscolha Nao para manter.
en.KeepData=Also remove your worlds (saves) and settings?%n%nChoose No to keep them.
es.KeepData=¿Eliminar tambien tus mundos (saves) y ajustes?%n%nElige No para conservarlos.
[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
[Files]
; atualizar sobrescreve o jogo, mas NUNCA saves/, logs/, profile.cfg, options.cfg, gamepad.cfg nem resourcepacks ja existentes
Source: "{#Stage}\*"; DestDir: "{app}"; Excludes: "saves\*,logs\*,crash-reports\*,profile.cfg,options.cfg,gamepad.cfg,assets\resourcepacks\*"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Stage}\assets\resourcepacks\*"; DestDir: "{app}\assets\resourcepacks"; Flags: onlyifdoesntexist recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "blockland.ico"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{autoprograms}\Blockland"; Filename: "{app}\blockland.exe"; WorkingDir: "{app}"; IconFilename: "{app}\blockland.ico"
Name: "{autodesktop}\Blockland"; Filename: "{app}\blockland.exe"; WorkingDir: "{app}"; IconFilename: "{app}\blockland.ico"; Tasks: desktopicon
[Run]
Filename: "{app}\blockland.exe"; WorkingDir: "{app}"; Description: "{cm:LaunchProgram,Blockland}"; Flags: nowait postinstall skipifsilent
[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    if MsgBox(CustomMessage('KeepData'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then begin
      DelTree(ExpandConstant('{app}\saves'), True, True, True);
      DelTree(ExpandConstant('{app}\logs'), True, True, True);
      DelTree(ExpandConstant('{app}\crash-reports'), True, True, True);
      DeleteFile(ExpandConstant('{app}\profile.cfg'));
      DeleteFile(ExpandConstant('{app}\options.cfg'));
      DeleteFile(ExpandConstant('{app}\gamepad.cfg'));
      RemoveDir(ExpandConstant('{app}'));
    end;
end;

function InitializeSetup(): Boolean;
var K, Loc: String;
begin
  // patchSetup.1: pasta apagada na mao => entrada orfa no registro; sem o exe la, limpa e instala do zero
  Result := True;
  K := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{B7C1E5D2-4A0F-4D8E-9C63-5E2B1F7A9D40}_is1';
  if RegQueryStringValue(HKCU, K, 'InstallLocation', Loc) then
    if not FileExists(AddBackslash(Loc) + 'blockland.exe') then
      RegDeleteKeyIncludingSubkeys(HKCU, K);
end;
