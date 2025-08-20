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
UninstallDisplayIcon={app}\MacType.exe
LicenseFile=upstream\original\license.txt

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; === x64 아키텍처 파일들 ===
Source: "x64\Rel+Detours\MacType64.Core.dll"; DestDir: "{app}\bin\x64"; DestName: "MacType64.Core.dll"; Flags: ignoreversion; Check: IsX64
Source: "x64\Release\macloader64.exe"; DestDir: "{app}\bin\x64"; DestName: "macloader64.exe"; Flags: ignoreversion; Check: IsX64

; === x86 아키텍처 파일들 ===
Source: "Rel+Detours\MacType.Core.dll"; DestDir: "{app}\bin\x86"; DestName: "MacType.Core.dll"; Flags: ignoreversion; Check: not IsX64
Source: "Release\MacLoader.exe"; DestDir: "{app}\bin\x86"; DestName: "MacLoader.exe"; Flags: ignoreversion; Check: not IsX64

; === 공통 실행 파일 (64비트 우선) ===
Source: "x64\Release\macloader64.exe"; DestDir: "{app}"; DestName: "MacType.exe"; Flags: ignoreversion; Check: IsX64
Source: "Release\MacLoader.exe"; DestDir: "{app}"; DestName: "MacType.exe"; Flags: ignoreversion; Check: not IsX64

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
Name: "{group}\Uninstall MacType"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\MacType.exe"; Description: "MacType 실행"; Flags: nowait postinstall skipifsilent

[UninstallRun]
; 서비스 제거는 수동으로 수행하거나 별도 스크립트로 처리

[Registry]
; 서비스 모드 활성화 (관리자 권한 필요)
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\MacType.exe"; ValueData: "RUNASADMIN"; Flags: uninsdeletevalue

; 설치 정보 등록
Root: HKLM; Subkey: "SOFTWARE\MacType"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\MacType"; ValueType: string; ValueName: "ConfigPath"; ValueData: "{app}\config"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\MacType"; ValueType: string; ValueName: "Version"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

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
