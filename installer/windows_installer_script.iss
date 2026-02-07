#define AppName "SpatGRIS"
#define AppPublisher "GRIS - UdeM"
#define AppURL "http://gris.musique.umontreal.ca/"
#define AppExeName "SpatGRIS.exe"
#define ControlGrisDir "ControlGRIS"
#define SpeakerViewDir "SpeakerView"
#define ManualENName "SpatGRIS_4.0.1_Manual_EN.pdf"
#define ManualFRName "SpatGRIS_4.0.1_Manuel_FR.pdf"
#define RootDir ".."

#define BuildDir RootDir + "\Builds\VisualStudio2022\x64\Release\App"
#define ControlGrisVersionLong GetVersionNumbersString(ControlGrisDir + "\ControlGRIS2.vst3\Contents\x86_64-win\ControlGRIS2.vst3")
#define SpeakerViewVersion GetFileVersion(SpeakerViewDir + "\SpeakerView.exe")
#define ResourcesDir RootDir + "\Resources"
#define AlgoGRISDir RootDir + "\submodules\AlgoGRIS"

#define AppExePath BuildDir + "\" + AppExeName
#define ControlGrisVersion Copy(ControlGrisVersionLong, 0, Len(ControlGrisVersionLong) - 2)
#define AaxInPath RootDir + "\installer\ControlGris\ControlGRIS2.aaxplugin"
#define AaxOutPath "C:\Program Files\Common Files\Avid\Audio\Plug-Ins\ControlGRIS2.aaxplugin"

#define AppVersionLong GetVersionNumbersString(AppExePath)

#define AppVersion Copy(AppVersionLong, 0, Len(AppVersionLong) - 2)

[Setup]
AppId={{5B626B1D-A428-406C-95A7-EFAE69ECD751}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
ArchitecturesInstallIn64BitMode=x64
Compression=lzma
DefaultDirName={autopf}\{#AppName}
DisableDirPage=no
DisableProgramGroupPage=yes
LicenseFile="..\LICENSE"
OutputDir=.
OutputBaseFilename="{#AppName}_{#AppVersion}_Windows_x64"
PrivilegesRequired=admin
SetupIconFile="{#ResourcesDir}\ServerGRIS_icon_doc.ico"
SolidCompression=yes
WizardStyle=modern
;UsePreviousAppDir=no
; Add the following line to run user mode (install for all users.)
;PrivilegesRequired=lowest
;PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

;[InstallDelete]
;Type: files; Name: "{commoncf64}\VST3\ControlGris.vst3"

[Files]
Source: "{#AppExePath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AlgoGRISDir}\hrtf_compact\*"; DestDir: "{app}\Resources\hrtf_compact"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ResourcesDir}\default_preset\*"; DestDir: "{app}\Resources\default_preset"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ResourcesDir}\templates\*"; DestDir: "{app}\Resources\templates"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ResourcesDir}\{#ManualENName}"; DestDir: "{app}\Resources"; Flags: ignoreversion
Source: "{#ResourcesDir}\{#ManualFRName}"; DestDir: "{app}\Resources"; Flags: ignoreversion
Source: "{#ResourcesDir}\ServerGRIS_icon_splash_small.png"; DestDir: "{app}\Resources"; Flags: ignoreversion
Source: "{#ResourcesDir}\splash_screen.png"; DestDir: "{app}\Resources"; Flags: ignoreversion

; ControlGRIS plugins
Source: "{#ControlGrisDir}\ControlGRIS2.vst3\*"; DestDir: "C:\Program Files\Common Files\VST3\ControlGRIS2.vst3"; Components: VST364; Flags: ignoreversion recursesubdirs createallsubdirs;
Source: "ControlGris\ControlGRIS2.aaxplugin\*"; DestDir: "C:\Program Files\Common Files\Avid\Audio\Plug-Ins\ControlGRIS2.aaxplugin"; Components: AAX; Flags: ignoreversion recursesubdirs createallsubdirs;

; ControlGRIS standalone
Source: "{#ControlGrisDir}\ControlGRIS2.exe"; DestDir: "{app}"; Components: ControlGRIS2Standalone; Flags: ignoreversion

; SpeakerView
Source: "{#SpeakerViewDir}\SpeakerView.exe"; DestDir: "{app}"; Components: SpeakerView; Flags: ignoreversion
Source: "{#SpeakerViewDir}\SpeakerView.pck"; DestDir: "{app}"; Components: SpeakerView; Flags: ignoreversion

[Components]
Name: "SpeakerView"; Description: SpeakerView {#SpeakerViewVersion}; Types: custom full;
Name: "VST364"; Description: "ControlGRIS {#ControlGrisVersion} VST3"; Types: custom full;
Name: "AAX"; Description: "ControlGRIS {#ControlGrisVersion} AAX"; Types: custom full;
Name: "ControlGRIS2Standalone"; Description: "ControlGRIS {#ControlGrisVersion} Standalone"; Types: custom full;

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon
Name: "{group}\Uninstall {#AppName} {#AppVersion}"; Filename: {uninstallexe}

[Run]
Filename: "{app}\Resources\{#ManualENName}"; WorkingDir: "{app}\Resources"; Description: "Open PDF User Manual (EN)"; Flags: shellexec postinstall skipifsilent nowait
Filename: "{app}\Resources\{#ManualFRName}"; WorkingDir: "{app}\Resources"; Description: "Ouvrir le Manuel PDF de l'Utilisateur (FR)"; Flags: shellexec postinstall skipifsilent nowait unchecked
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
  begin
    if WizardIsComponentSelected('VST364') then
    begin
      DeleteFile('C:\Program Files\Common Files\VST3\ControlGRIS2.vst3');
    end;
  end;
end;
