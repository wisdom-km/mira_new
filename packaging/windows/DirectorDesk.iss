#define AppName "DirectorDesk"
#define AppVersion "0.1.1"
#define AppPublisher "DirectorDesk contributors"
#define AppURL "https://github.com/wisdom-km/mira_new"
#define AppExeName "DirectorDesk.exe"

[Setup]
AppId={{8F3A1C2E-9B70-4D5A-8E21-4C6F0A1B2D3E}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir=..\..\dist
OutputBaseFilename=DirectorDesk-{#AppVersion}-windows-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#AppExeName}
CloseApplications=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
Source: "..\stage\DirectorDesk\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Parameters: "--project ""{app}\examples\cafe.ddproj"""
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Parameters: "--project ""{app}\examples\cafe.ddproj"""; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Parameters: "--project ""{app}\examples\cafe.ddproj"""; Description: "Launch DirectorDesk"; Flags: nowait postinstall skipifsilent
