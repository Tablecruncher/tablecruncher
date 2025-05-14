; Tablecruncher Installer Script with Uninstall and File Association

[Setup]
AppName=Tablecruncher
AppVersion={#MyAppVersion}
DefaultDirName={pf}\Tablecruncher
DefaultGroupName=Tablecruncher
OutputDir=.
OutputBaseFilename=TablecruncherInstaller
Compression=lzma
SolidCompression=yes
UninstallDisplayIcon={app}\Tablecruncher.exe

[Files]
Source: "Tablecruncher.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Tablecruncher"; Filename: "{app}\Tablecruncher.exe"
Name: "{commondesktop}\Tablecruncher"; Filename: "{app}\Tablecruncher.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"

[Run]
Filename: "{app}\Tablecruncher.exe"; Description: "Launch Tablecruncher"; Flags: nowait postinstall skipifsilent

[Registry]
; Register CSV file association
Root: HKCR; Subkey: ".csv"; ValueType: string; ValueName: ""; ValueData: "Tablecruncher.csv"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "Tablecruncher.csv"; ValueType: string; ValueName: ""; ValueData: "CSV File (Tablecruncher)"; Flags: uninsdeletekey
Root: HKCR; Subkey: "Tablecruncher.csv\DefaultIcon"; ValueType: string; ValueData: "{app}\Tablecruncher.exe,0"
Root: HKCR; Subkey: "Tablecruncher.csv\shell\open\command"; ValueType: string; ValueData: """{app}\Tablecruncher.exe"" ""%1"""
