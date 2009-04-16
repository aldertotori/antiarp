; �ű��� Inno Setup �ű��� ���ɣ�
; �йش��� Inno Setup �ű��ļ�����ϸ��������İ����ĵ���

[Setup]
AppName=��� AntiArp
AppVerName=��� AntiArp For 2000/XP/2003/Vista/2008
AppPublisher=Debugman.com
AppPublisherURL=http://www.Debugman.com
AppSupportURL=http://www.Debugman.com
AppUpdatesURL=http://www.Debugman.com
DefaultDirName={pf}\��� AntiArp
DefaultGroupName=��� AntiArp
OutputBaseFilename=WonArp
SetupIconFile=..\WonFW\res\WonArpFW.ico
Compression=lzma
SolidCompression=yes
PrivilegesRequired = admin

MinVersion=5.0,5.0
OnlyBelowVersion=5.0,6.0

ArchitecturesAllowed = x86
VersionInfoCompany=Debugman.com
VersionInfoVersion=1.0.0.1
VersionInfoTextVersion=1.0.0.1
VersionInfoCopyright=Debugman.com
VersionInfoDescription=��� AntiArp For 2000/XP/2003/Vista/2008
UninstallRestartComputer = yes

[Languages]
Name: "chinese"; MessagesFile: "compiler:Default.isl"
Name: "english"; MessagesFile: "compiler:Languages\English.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; ����ǽ������
Source: "..\Release\WonFW.exe";      DestDir: "{app}"; Flags: ignoreversion  uninsrestartdelete onlyifdoesntexist
Source: "..\Release\WonArp.dll";     DestDir: "{app}"; Flags: ignoreversion  uninsrestartdelete onlyifdoesntexist

; 2k/XP/2003 Driver
Source: "..\Release\PTWONARP.inf";     DestDir: "{app}"; Flags: ignoreversion  deleteafterinstall onlyifdoesntexist
Source: "..\Release\MPWONARP.inf";     DestDir: "{app}"; Flags: ignoreversion  deleteafterinstall onlyifdoesntexist
Source: "..\Release\WonArp.sys";       DestDir: "{app}"; Flags: ignoreversion  deleteafterinstall onlyifdoesntexist

; Vista Driver
;Source: "..\Release\WonArp6.inf";     DestDir: "{app}"; Flags: ignoreversion  deleteafterinstall onlyifdoesntexist
;Source: "..\Release\WonArp6.sys";     DestDir: "{app}"; Flags: ignoreversion  deleteafterinstall onlyifdoesntexist


[Icons]
Name: "{group}\��� AntiArp"; Filename: "{app}\WonFW.exe.exe"
Name: "{commondesktop}\��� AntiArp"; Filename: "{app}\WonFW.exe"; Tasks: desktopicon

[Registry]
; ϵͳ������
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "WonArpFW"; ValueData: "{app}\WonFW.exe /Auto" ; Flags : uninsdeletevalue
; ж��ɾ��ע�����Ŀ
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Services\WonArp"; Flags: uninsdeletekey dontcreatekey noerror

[Run]

; ��������ǩ��
Filename: "{app}\WonFW.exe"; Parameters: "/DisableSign" ; Flags : runhidden
; ��װ����
Filename: "{app}\WonFW.exe"; Parameters: "/Install" ; Flags : runhidden ; StatusMsg: "���ڰ�װ ��� AntiArp�����Ժ�..."
; ��������ǩ��
Filename: "{app}\WonFW.exe"; Parameters: "/EnableSign" ; Flags : runhidden

Filename: "{app}\WonFW.exe"; Description: "{cm:LaunchProgram,��� AntiArp}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
; ж������
Filename: "{app}\WonFW.exe"; Parameters: "/Remove" ; Flags : runhidden  ; StatusMsg: "����ж����� AntiArp,���ֶ��ر��������еķ���ǽ����..."




