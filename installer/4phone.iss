#ifndef AppVersion
  #define AppVersion "3.22.3-4phone.1"
#endif

#ifndef AppVersionNumeric
  #define AppVersionNumeric "3.22.3.6"
#endif

#ifndef SourceDir
  #define SourceDir "..\microsip\Release"
#endif

#ifndef OutputDir
  #define OutputDir "..\microsip\Release"
#endif

#define AppName "4phone"
#define AppPublisher "4phone.uz"
#define AppExeName "4phone.exe"

[Setup]
AppId={{FE754C0A-1D14-4904-AE92-33338B508B71}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL=https://4phone.uz/
AppSupportURL=https://4phone.uz/
AppUpdatesURL=https://4phone.uz/
DefaultDirName={localappdata}\Programs\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir={#OutputDir}
OutputBaseFilename=4phone-setup
SetupIconFile=..\microsip\res\4phone.ico
WizardImageFile=4phone-wizard.bmp
WizardSmallImageFile=4phone-wizard-small.bmp
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
CloseApplicationsFilter={#AppExeName}
RestartApplications=no
MinVersion=6.1sp1
VersionInfoVersion={#AppVersionNumeric}
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} Windows Installer
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersionNumeric}
VersionInfoProductTextVersion={#AppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Dirs]
Name: "{userappdata}\{#AppName}"; Flags: uninsneveruninstall
Name: "{localappdata}\{#AppName}"; Flags: uninsneveruninstall

[Files]
Source: "{#SourceDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\WinSparkle.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\langpack_russian.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\langpack_uzbek.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\PRIVACY.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\SECURITY.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{src}\4phone.ini"; DestDir: "{userappdata}\{#AppName}"; Flags: external skipifsourcedoesntexist onlyifdoesntexist uninsneveruninstall
Source: "{userdesktop}\4phone.ini"; DestDir: "{userappdata}\{#AppName}"; Flags: external skipifsourcedoesntexist onlyifdoesntexist uninsneveruninstall
Source: "{src}\Contacts.xml"; DestDir: "{userappdata}\{#AppName}"; Flags: external skipifsourcedoesntexist onlyifdoesntexist uninsneveruninstall
Source: "{userdesktop}\Contacts.xml"; DestDir: "{userappdata}\{#AppName}"; Flags: external skipifsourcedoesntexist onlyifdoesntexist uninsneveruninstall

[Registry]
Root: HKCU; Subkey: "Software\{#AppName}"; ValueType: string; ValueName: ""; ValueData: "{app}"; Flags: uninsdeletekey

[Icons]
Name: "{userprograms}\{#AppName}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{userdesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent
