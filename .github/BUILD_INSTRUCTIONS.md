# GitHub Actions 빌드 설명서

이 문서는 GitHub Actions를 통한 MacType의 자동 빌드 환경에 대해 설명합니다.

## 개요

이 리포지토리는 다음과 같은 자동화된 빌드 시스템을 제공합니다:

1. **메인 빌드 워크플로우** (`.github/workflows/build.yml`)
   - 코드가 푸시되거나 PR이 생성될 때마다 실행
   - x86과 x64 플랫폼을 지원
   - 모든 종속성을 자동으로 빌드하고 설정

2. **릴리스 워크플로우** (`.github/workflows/release.yml`)
   - 태그가 푸시될 때 실행
   - 완성된 바이너리를 GitHub Release로 자동 배포

## 빌드 프로세스

### 1. 종속성 빌드

빌드 프로세스는 `scripts/build-dependencies.ps1` 스크립트를 사용하여 다음 종속성들을 자동으로 빌드합니다:

#### FreeType
- 사용자 정의 FreeType 버전: https://github.com/snowie2000/freetype
- `glyph_to_bitmapex.diff` 패치 적용
- x86: `freetype.lib`, x64: `freetype64.lib`로 빌드

#### IniParser
- 포크 버전: https://github.com/snowie2000/IniParser  
- x86: `iniparser.lib`, x64: `iniparser64.lib`로 빌드

#### wow64ext
- 포크 버전: https://github.com/snowie2000/rewolf-wow64ext
- x86 플랫폼에서만 필요: `wow64ext.lib`

#### Detours
- Microsoft Detours: https://github.com/microsoft/Detours
- x86: `detours.lib`, x64: `detours64.lib`로 빌드

### 2. MacType 빌드

모든 종속성이 `lib/` 폴더에 준비되면 MSBuild를 사용하여 MacType을 빌드합니다.

## 로컬 빌드

로컬에서 종속성을 빌드하려면:

```powershell
# PowerShell 실행 정책 설정
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# x86 빌드
.\scripts\build-dependencies.ps1 -Platform x86

# x64 빌드  
.\scripts\build-dependencies.ps1 -Platform x64

# MacType 빌드
msbuild gdipp.sln -p:Configuration=Release -p:Platform=Win32  # x86용
msbuild gdipp.sln -p:Configuration=Release -p:Platform=x64    # x64용
```

## 캐싱

빌드 시간을 단축하기 위해 GitHub Actions는 다음을 캐시합니다:
- `lib/` 폴더의 빌드된 라이브러리들
- `deps/` 폴더의 종속성 소스 코드

캐시는 `HOWTOBUILD.md` 파일이 변경될 때마다 무효화됩니다.

## 릴리스 생성

새 릴리스를 생성하려면:

1. 버전 태그를 생성하고 푸시:
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

2. GitHub Actions가 자동으로:
   - 모든 종속성을 빌드
   - MacType을 x86과 x64로 빌드
   - 빌드 결과물을 압축
   - GitHub Release를 생성하고 파일을 첨부

## 문제 해결

### 빌드 실패 시 확인사항

1. **종속성 빌드 실패**
   - GitHub Actions 로그에서 어떤 종속성에서 실패했는지 확인
   - 해당 종속성의 리포지토리가 접근 가능한지 확인
   - 패치 파일이 올바르게 적용되는지 확인

2. **MacType 빌드 실패**
   - 모든 `.lib` 파일이 `lib/` 폴더에 있는지 확인
   - MSBuild 플랫폼 설정이 올바른지 확인
   - 환경 변수가 올바르게 설정되었는지 확인

3. **권한 오류**
   - PowerShell 실행 정책 설정이 올바른지 확인
   - 스크립트 파일이 올바른 경로에 있는지 확인

### 캐시 문제

캐시로 인한 문제가 발생하면 GitHub Actions에서 캐시를 수동으로 삭제할 수 있습니다:
1. 리포지토리의 Actions 탭으로 이동
2. 좌측 사이드바에서 "Caches" 선택
3. 문제가 되는 캐시를 삭제

## 환경 변수

빌드 프로세스에서 사용되는 환경 변수:

- `FREETYPE_PATH`: FreeType 소스 경로
- `INI_PARSER_PATH`: IniParser 소스 경로
- `BUILD_CONFIGURATION`: 빌드 구성 (기본값: Release)

## 기여하기

빌드 시스템을 개선하거나 문제를 발견한 경우:

1. 이슈를 생성하여 문제를 보고해주세요
2. PR을 통해 개선사항을 제안해주세요
3. 새로운 종속성이 추가되는 경우 `build-dependencies.ps1` 스크립트를 업데이트해주세요

## 지원되는 환경

- **CI/CD**: GitHub Actions (Windows runner)
- **빌드 도구**: MSBuild, Visual Studio 2019+ 툴킷
- **플랫폼**: x86, x64
- **구성**: Release (기본), Debug 가능
