; Inno Setup Script for MacType Universal (x64, Detours)
; 빌드 방법(Windows):
;  1) choco install innosetup -y  (또는 공식 Inno Setup 설치)
;  2) iscc installer\mactype.iss

#define MyAppName "MacType Universal"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "YourOrg"
#define MyAppURL "https://github.com/Meapri/mactype-universal"
#define MyAppExeName "macloader64.exe"

; 이 스크립트는 repo 루트에서 컴파일된 산출물을 사용합니다.
; 폴더 구조:
;   x64\Rel+Detours\MacType64.Core.dll
;   x64\Release\macloader64.exe

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
OutputBaseFilename=MacType-Setup-x64
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
WizardStyle=modern

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Core/Loader
Source: "..\x64\Rel+Detours\MacType64.Core.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\x64\Release\{#MyAppExeName}"; DestDir: "{app}\bin"; Flags: ignoreversion

; 설정/프로필(있는 경우만)
Source: "..\installer\upstream\config\*.ini"; DestDir: "{app}\config"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: DirExists(ExpandConstant('..\installer\upstream\config'))
Source: "..\config\*.ini"; DestDir: "{app}\config"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: DirExists(ExpandConstant('..\config'))

[Icons]
Name: "{group}\MacType Loader"; Filename: "{app}\bin\{#MyAppExeName}"
Name: "{group}\Uninstall MacType"; Filename: "{uninstallexe}"
Name: "{autodesktop}\MacType Loader"; Filename: "{app}\bin\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "바탕화면에 바로가기 만들기"; GroupDescription: "추가 작업 선택:"; Flags: unchecked

[Run]
; IFEO 등록(선택 시)
Filename: "{app}\bin\{#MyAppExeName}"; Parameters: "/register"; Flags: runhidden; Tasks: ifeo

[UninstallRun]
; 제거 시 IFEO 해제(선택 시)
Filename: "{app}\bin\{#MyAppExeName}"; Parameters: "/unregister"; Flags: runhidden; Tasks: ifeo

[Registry]
; IFEO 등 레지스트리를 만질 경우 ‘Tasks: ifeo’ 조건으로 선택적 적용 권장
; 예시(비활성):
; Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe"; ValueType: string; ValueName: "VerifierDlls"; ValueData: "{app}\bin\MacType64.Core.dll"; Flags: uninsdeletekey; Tasks: ifeo

[Tasks]
Name: "ifeo"; Description: "IFEO 기반 등록(고급)"; GroupDescription: "고급 설정:"; Flags: unchecked

[Code]
function DirExists(const Dir: string): Boolean;
begin
  Result := Dir <> '';
  if Result then
    Result := FileExists(ExpandConstant(Dir + '\*')) or 
              (Pos('..', Dir) = 1) or 
              (Pos(':', Dir) = 2);
end;


