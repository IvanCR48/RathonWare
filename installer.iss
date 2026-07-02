; Inno Setup Script for RathonWare
; This script compiles the packaged files in the 'dist' directory into a single installer.

#define MyAppName "RathonWare"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Rathon"
#define MyAppURL "https://github.com/IvanCR48/RathonWare"
#define MyAppExeName "RathonWare.exe"

[Setup]
; Unique App Id (generated randomly for this app)
AppId={{D37D52F7-C1E9-4E90-BA3D-1FEA28C9F80D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
; LicenseFile=LICENSE ; Uncomment if you add a LICENSE file
; InfoBeforeFile=README.md ; Uncomment if you want to display README before install
OutputDir=build
OutputBaseFilename=RathonWareSetup
SetupIconFile=assets\logo.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Source files should be collected in the 'dist' folder before running this script
Source: "dist\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Avoid repeating the main exe which is already caught by the wildcard, but ignoreversion helps ensure it matches.

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
