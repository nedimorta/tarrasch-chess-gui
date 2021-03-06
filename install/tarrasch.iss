; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=TarraschDb Chess GUI
AppVerName=TarraschDb Chess GUI V3 Beta
AppPublisher=Triple Happy Ltd.
AppPublisherURL=http://www.triplehappy.com
AppSupportURL=http://www.triplehappy.com
AppUpdatesURL=http://www.triplehappy.com
DefaultDirName={pf}\TarraschDb
DefaultGroupName=TarraschDb
LicenseFile=licence.txt
OutputDir=.
OutputBaseFilename=setup
Compression=lzma
SolidCompression=yes
ChangesAssociations=no

[Registry]

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "TarraschDb.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Engines\TarraschToyEngine.exe"; DestDir: "{app}\Engines"; Flags: ignoreversion
Source: "Engines\Rybka v2.3.2a.mp.w32.exe"; DestDir: "{app}\Engines"; Flags: ignoreversion
Source: "Engines\komodo3-32.exe"; DestDir: "{app}\Engines"; Flags: ignoreversion
Source: "Engines\komodo3-64.exe"; DestDir: "{app}\Engines"; Flags: ignoreversion
Source: "Engines\komodo3-64-sse.exe"; DestDir: "{app}\Engines"; Flags: ignoreversion
Source: "Engines\stockfish 7 x64.exe"; DestDir: "{app}\Engines"; Flags: ignoreversion
Source: "Engines\stockfish 7 32bit.exe"; DestDir: "{app}\Engines"; Flags: ignoreversion
Source: "Engines\Houdini_15a_w32.exe"; DestDir: "{app}\Engines"; Flags: ignoreversion
Source: "Engines\Houdini_15a_x64.exe"; DestDir: "{app}\Engines"; Flags: ignoreversion
Source: "book.pgn"; DestDir: "{app}"; Flags: ignoreversion
Source: "book.pgn_compiled"; DestDir: "{app}"; Flags: ignoreversion
Source: "kingbase-lite-2016-03.tdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "web.zip"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\TarraschDb"; Filename: "{app}\TarraschDb.exe"
Name: "{userdesktop}\TarraschDb"; Filename: "{app}\TarraschDb"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\TarraschDb"; Filename: "{app}\TarraschDb.exe"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\TarraschDb.exe"; Description: "{cm:LaunchProgram,TarraschDb}"; Flags: nowait postinstall skipifsilent

