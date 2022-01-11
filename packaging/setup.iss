#define MyAppName "Celeste"
#define MyAppVersion "1.0"
#define MyAppPublisher "The Observation Deck"
#define MyAppURL "https://github.com/herbmillerjr/Celeste"
#define MyAppExeName "Celeste.exe"

[Setup]
AppId={{E0BBEFE0-640F-40D9-BB9D-C859E51D087D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
OutputBaseFilename=SetupCeleste
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "C:\src\Celeste\packaging\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\libcrypto-1_1-x64.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\libEGL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\libGLESv2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\libssl-1_1-x64.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\Qt5Multimedia.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\Qt5MultimediaWidgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\Qt5Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\Qt5WinExtras.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\src\Celeste\packaging\audio\*"; DestDir: "{app}\audio"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\src\Celeste\packaging\bearer\*"; DestDir: "{app}\bearer"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\src\Celeste\packaging\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\src\Celeste\packaging\mediaservice\*"; DestDir: "{app}\mediaservice"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\src\Celeste\packaging\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\src\Celeste\packaging\playlistformats\*"; DestDir: "{app}\playlistformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\src\Celeste\packaging\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

