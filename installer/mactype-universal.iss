; Inno Setup Script for MacType Universal (x86/x64, JSON Configuration)
; 빌드 방법(Windows):
;  1) choco install innosetup -y  (또는 공식 Inno Setup 설치)
;  2) iscc installer\mactype-universal.iss

#define MyAppName "MacType Universal"
#define MyAppVersion "2025.01.20"
#define MyAppPublisher "MacType Community"
#define MyAppURL "https://github.com/Meapri/mactype-universal"
#define MyAppExeName "macloader64.exe"

[Setup]
AppId={{3E0F8B8E-0B1B-4A40-9F46-5A2B8F9912E6}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={pf}\MacType
DefaultGroupName=MacType
ArchitecturesInstallIn64BitMode=x64
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=MacType-Universal-Installer
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
WizardStyle=modern
SetupIconFile=upstream\original\icons.dll
UninstallDisplayIcon={app}\bin\macloader64.exe
LicenseFile=upstream\original\license.txt

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; === x64 아키텍처 파일들 ===
Source: "Files\x64\MacType64.Core.dll"; DestDir: "{app}\bin\x64"; DestName: "MacType64.Core.dll"; Flags: ignoreversion; Check: IsX64
Source: "Files\x64\macloader64.exe"; DestDir: "{app}\bin\x64"; DestName: "macloader64.exe"; Flags: ignoreversion; Check: IsX64
Source: "Files\x64\MacTray.exe"; DestDir: "{app}\bin\x64"; DestName: "MacTray.exe"; Flags: ignoreversion; Check: IsX64
Source: "Files\x64\MacTuner.exe"; DestDir: "{app}\bin\x64"; DestName: "MacTuner.exe"; Flags: ignoreversion; Check: IsX64
Source: "Files\x64\MacWiz.exe"; DestDir: "{app}\bin\x64"; DestName: "MacWiz.exe"; Flags: ignoreversion; Check: IsX64
Source: "Files\x64\mt64agnt.exe"; DestDir: "{app}\bin\x64"; DestName: "mt64agnt.exe"; Flags: ignoreversion; Check: IsX64

; === x86 아키텍처 파일들 ===
Source: "Files\x86\MacType.Core.dll"; DestDir: "{app}\bin\x86"; DestName: "MacType.Core.dll"; Flags: ignoreversion; Check: not IsX64
Source: "Files\x86\MacLoader.exe"; DestDir: "{app}\bin\x86"; DestName: "MacLoader.exe"; Flags: ignoreversion; Check: not IsX64
Source: "Files\x86\MacTray.exe"; DestDir: "{app}\bin\x86"; DestName: "MacTray.exe"; Flags: ignoreversion; Check: not IsX64
Source: "Files\x86\MacTuner.exe"; DestDir: "{app}\bin\x86"; DestName: "MacTuner.exe"; Flags: ignoreversion; Check: not IsX64
Source: "Files\x86\MacWiz.exe"; DestDir: "{app}\bin\x86"; DestName: "MacWiz.exe"; Flags: ignoreversion; Check: not IsX64
Source: "Files\x86\mt64agnt.exe"; DestDir: "{app}\bin\x86"; DestName: "mt64agnt.exe"; Flags: ignoreversion; Check: not IsX64

; === 공통 실행 파일 (64비트 우선) ===
Source: "Files\x64\macloader64.exe"; DestDir: "{app}"; DestName: "MacType.exe"; Flags: ignoreversion; Check: IsX64
Source: "Files\x86\MacLoader.exe"; DestDir: "{app}"; DestName: "MacType.exe"; Flags: ignoreversion; Check: not IsX64

; === 설정 파일들 (JSON 우선) ===
; 기본 설정 파일들
Source: "upstream\original\config\*.json"; DestDir: "{app}\config"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "upstream\original\config\*.ini"; DestDir: "{app}\config"; Flags: ignoreversion recursesubdirs createallsubdirs

; 언어 파일들
Source: "upstream\original\languages\*.lng"; DestDir: "{app}\languages"; Flags: ignoreversion

; 문서 파일들
Source: "upstream\original\license.txt"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "upstream\original\ChangeLog.txt"; DestDir: "{app}"; DestName: "ChangeLog.txt"; Flags: ignoreversion

; 아이콘 및 리소스
Source: "upstream\original\icons.dll"; DestDir: "{app}\resources"; Flags: ignoreversion

[Icons]
Name: "{group}\MacType"; Filename: "{app}\MacType.exe"
Name: "{group}\MacType Tuner"; Filename: "{app}\bin\x64\MacTuner.exe"; Check: IsX64
Name: "{group}\MacType Tuner"; Filename: "{app}\bin\x86\MacTuner.exe"; Check: not IsX64
Name: "{group}\MacType Tray"; Filename: "{app}\bin\x64\MacTray.exe"; Check: IsX64
Name: "{group}\MacType Tray"; Filename: "{app}\bin\x86\MacTray.exe"; Check: not IsX64
Name: "{group}\MacType Agent"; Filename: "{app}\bin\x64\mt64agnt.exe"; Check: IsX64
Name: "{group}\MacType Agent"; Filename: "{app}\bin\x86\mt64agnt.exe"; Check: not IsX64
Name: "{group}\Uninstall MacType"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\bin\x64\mt64agnt.exe"; Parameters: "/install"; Check: IsX64; Flags: runhidden
Filename: "{app}\bin\x86\mt64agnt.exe"; Parameters: "/install"; Check: not IsX64; Flags: runhidden
Filename: "{app}\MacType.exe"; Description: "MacType 실행"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\bin\x64\mt64agnt.exe"; Parameters: "/uninstall"; Check: IsX64; Flags: runhidden
Filename: "{app}\bin\x86\mt64agnt.exe"; Parameters: "/uninstall"; Check: not IsX64; Flags: runhidden

[Registry]
; 서비스 모드 활성화 (관리자 권한 필요)
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\MacType.exe"; ValueData: "RUNASADMIN"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\bin\x64\MacType64.Core.dll"; ValueData: "RUNASADMIN"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\bin\x86\MacType.Core.dll"; ValueData: "RUNASADMIN"; Flags: uninsdeletevalue

; DLL 경로 등록
Root: HKLM; Subkey: "SOFTWARE\MacType"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\MacType"; ValueType: string; ValueName: "ConfigPath"; ValueData: "{app}\config"; Flags: uninsdeletekey

[Code]
function IsX64: Boolean;
begin
  Result := Is64BitInstallMode;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
end;

function InitializeUninstall(): Boolean;
begin
  Result := True;
end;

[Messages]
; 한글 메시지
WelcomeLabel1=MacType Universal 설치를 시작합니다.
WelcomeLabel2=이 설치 프로그램은 컴퓨터에 MacType을 설치합니다.%n%nMacType은 Windows에서 폰트 렌더링을 개선하는 프로그램입니다.

; 영어 메시지
; WelcomeLabel1=Welcome to the MacType Universal Setup Wizard.
; WelcomeLabel2=This will install MacType on your computer.%n%nMacType improves font rendering on Windows systems.
