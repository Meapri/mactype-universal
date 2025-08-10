# MacType 현대적 종속성 빌드 스크립트 (vcpkg 기반)
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("x86", "x64", "ARM64")]
    [string]$Platform,
    
    [Parameter(Mandatory=$false)]
    [string]$Configuration = "Release",
    
    [Parameter(Mandatory=$false)]
    [string]$VcpkgRoot = "$env:VCPKG_ROOT",
    
    [Parameter(Mandatory=$false)]
    [switch]$UseSystemVcpkg = $false,
    
    [Parameter(Mandatory=$false)]
    [switch]$SkipVcpkgInstall = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=== MacType 현대적 종속성 빌드 시작 ===" -ForegroundColor Green
Write-Host "플랫폼: $Platform, 구성: $Configuration" -ForegroundColor Cyan

# 디렉터리 설정
$rootDir = Get-Location
$libDir = Join-Path $rootDir "lib"
$depsDir = Join-Path $rootDir "deps"
$vcpkgManifest = Join-Path $rootDir "vcpkg.json"

# 디렉터리 생성
New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $depsDir | Out-Null

# vcpkg 설정 확인
if (-not $VcpkgRoot -or -not (Test-Path $VcpkgRoot)) {
    # GitHub Actions에서 이미 설치된 vcpkg 사용
    $possiblePaths = @(
        (Join-Path $rootDir "vcpkg"),
        "$env:VCPKG_ROOT",
        (Join-Path $depsDir "vcpkg")
    )
    
    foreach ($path in $possiblePaths) {
        if ($path -and (Test-Path $path)) {
            $VcpkgRoot = $path
            Write-Host "vcpkg 발견: $VcpkgRoot" -ForegroundColor Green
            break
        }
    }
    
    if (-not $VcpkgRoot -or -not (Test-Path $VcpkgRoot)) {
        if ($UseSystemVcpkg) {
            Write-Host "시스템 vcpkg 경로를 찾는 중..." -ForegroundColor Yellow
            $VcpkgRoot = (Get-Command vcpkg -ErrorAction SilentlyContinue).Source | Split-Path
            if (-not $VcpkgRoot) {
                throw "vcpkg를 찾을 수 없습니다. vcpkg를 설치하거나 VCPKG_ROOT 환경 변수를 설정하세요."
            }
        } else {
            # vcpkg를 로컬에 설치
            $VcpkgRoot = Join-Path $depsDir "vcpkg"
            if (-not (Test-Path $VcpkgRoot)) {
                Write-Host "vcpkg 다운로드 중..." -ForegroundColor Yellow
                git clone https://github.com/Microsoft/vcpkg.git $VcpkgRoot
                Set-Location $VcpkgRoot
                if ($IsWindows -or $env:OS -eq "Windows_NT") {
                    .\bootstrap-vcpkg.bat
                } else {
                    ./bootstrap-vcpkg.sh
                }
                Set-Location $rootDir
            }
        }
    }
}

$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    $vcpkgExe = Join-Path $VcpkgRoot "vcpkg"
}

if (-not (Test-Path $vcpkgExe)) {
    throw "vcpkg 실행파일을 찾을 수 없습니다: $vcpkgExe"
}

Write-Host "vcpkg 경로: $VcpkgRoot" -ForegroundColor Green

# 플랫폼별 triplet 설정
$triplet = switch ($Platform) {
    "x86" { "x86-windows" }
    "x64" { "x64-windows" }
    "ARM64" { "arm64-windows" }
    default { "x64-windows" }
}

Write-Host "사용할 triplet: $triplet" -ForegroundColor Cyan

# vcpkg 매니페스트 모드로 의존성 설치
Write-Host "vcpkg 의존성 설치 중..." -ForegroundColor Yellow
try {
    & $vcpkgExe install --triplet $triplet --x-manifest-root="$rootDir" --x-install-root="$depsDir/vcpkg_installed"
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg install 실패"
    }
    Write-Host "vcpkg 의존성 설치 완료" -ForegroundColor Green
} catch {
    Write-Host "vcpkg 설치 실패: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "매니페스트 모드에서는 개별 패키지 설치가 지원되지 않습니다." -ForegroundColor Yellow
    Write-Host "vcpkg.json의 패키지 버전을 확인해주세요." -ForegroundColor Yellow
    throw "vcpkg 의존성 설치 실패"
}

# 라이브러리 복사
Write-Host "라이브러리 파일 복사 중..." -ForegroundColor Yellow

$vcpkgInstallPath = Join-Path $depsDir "vcpkg_installed/$triplet"
$vcpkgLibPath = Join-Path $vcpkgInstallPath "lib"
$vcpkgDebugLibPath = Join-Path $vcpkgInstallPath "debug/lib"

if (-not (Test-Path $vcpkgInstallPath)) {
    # 시스템 vcpkg 경로 시도
    $vcpkgInstallPath = Join-Path $VcpkgRoot "installed/$triplet"
    $vcpkgLibPath = Join-Path $vcpkgInstallPath "lib"
    $vcpkgDebugLibPath = Join-Path $vcpkgInstallPath "debug/lib"
}

# 플랫폼별 라이브러리 이름 설정
$libSuffix = switch ($Platform) {
    "x86" { "" }
    "x64" { "64" }
    "ARM64" { "_arm64" }
    default { "64" }
}

# FreeType 라이브러리 복사
$sourceLibPath = if ($Configuration -eq "Debug") { $vcpkgDebugLibPath } else { $vcpkgLibPath }

if (Test-Path $sourceLibPath) {
    # FreeType
    $freetypeLib = Get-ChildItem -Path $sourceLibPath -Filter "*freetype*" -File | Select-Object -First 1
    if ($freetypeLib) {
        $targetFreetypeLib = Join-Path $libDir "freetype$libSuffix.lib"
        Copy-Item $freetypeLib.FullName $targetFreetypeLib -Force
        Write-Host "FreeType 라이브러리 복사 완료: $targetFreetypeLib" -ForegroundColor Green
    }
    
    # Detours
    $detoursLib = Get-ChildItem -Path $sourceLibPath -Filter "*detours*" -File | Select-Object -First 1
    if ($detoursLib) {
        $targetDetoursLib = Join-Path $libDir "detours$libSuffix.lib"
        Copy-Item $detoursLib.FullName $targetDetoursLib -Force
        Write-Host "Detours 라이브러리 복사 완료: $targetDetoursLib" -ForegroundColor Green
        
        # MacType이 요구하는 easyhk 이름으로도 복사
        $targetEasyhookLib = Join-Path $libDir "easyhk$libSuffix.lib"
        Copy-Item $detoursLib.FullName $targetEasyhookLib -Force
        Write-Host "EasyHook 호환 라이브러리 생성: $targetEasyhookLib" -ForegroundColor Green
    }
} else {
    Write-Host "vcpkg 라이브러리 경로를 찾을 수 없습니다: $sourceLibPath" -ForegroundColor Yellow
}

# 인클루드 헤더 복사
$vcpkgIncludePath = Join-Path $vcpkgInstallPath "include"
if (Test-Path $vcpkgIncludePath) {
    $targetIncludePath = Join-Path $depsDir "include"
    if (Test-Path $targetIncludePath) {
        Remove-Item $targetIncludePath -Recurse -Force
    }
    Copy-Item $vcpkgIncludePath $targetIncludePath -Recurse -Force
    Write-Host "헤더 파일 복사 완료: $targetIncludePath" -ForegroundColor Green
    
    # 환경 변수 설정 (GitHub Actions용)
    if ($env:GITHUB_ENV) {
        echo "VCPKG_INCLUDE_PATH=$targetIncludePath" >> $env:GITHUB_ENV
        echo "VCPKG_LIB_PATH=$libDir" >> $env:GITHUB_ENV
    }
}

# 추가 의존성 빌드 (vcpkg에서 지원하지 않는 것들)
Write-Host "추가 의존성 빌드 중..." -ForegroundColor Yellow

# IniParser (C# 라이브러리이므로 별도 처리)
$iniparserDir = Join-Path $depsDir "ini-parser"
if (-not (Test-Path $iniparserDir)) {
    Write-Host "INI 파서 다운로드 중..." -ForegroundColor Cyan
    git clone https://github.com/rickyah/ini-parser.git $iniparserDir
}

# INI 파서는 .NET/C# 라이브러리이므로 MacType에서 직접 사용하지 않을 수 있음
# 대신 간단한 C++ INI 파서를 생성하거나 다른 대안 사용

# wow64ext (32비트에서만 필요)
if ($Platform -eq "x86") {
    $wow64extDir = Join-Path $depsDir "wow64ext"
    if (-not (Test-Path $wow64extDir)) {
        Write-Host "wow64ext 다운로드 중..." -ForegroundColor Cyan
        git clone https://github.com/rwfpl/rewolf-wow64ext.git $wow64extDir
    }
    
    # wow64ext 빌드 (필요한 경우)
    Set-Location $wow64extDir
    if (Test-Path "src") {
        Write-Host "wow64ext 빌드 중..." -ForegroundColor Cyan
        # 프로젝트 파일이 있는 경우 MSBuild 사용
        $wow64extProject = Get-ChildItem -Recurse -Filter "*.vcxproj" | Select-Object -First 1
        if ($wow64extProject) {
            $sdkVersion = if ($env:WINDOWS_SDK_VERSION) { $env:WINDOWS_SDK_VERSION } else { "10.0.26100.0" }
            msbuild $wow64extProject.FullName -p:Configuration=$Configuration -p:Platform=Win32 -p:WindowsTargetPlatformVersion=$sdkVersion -p:PlatformToolset=v143 -v:minimal
            
            # 라이브러리 복사
            $wow64extLib = Get-ChildItem -Recurse -Filter "wow64ext*.lib" | Select-Object -First 1
            if ($wow64extLib) {
                $targetWow64extLib = Join-Path $libDir "wow64ext.lib"
                Copy-Item $wow64extLib.FullName $targetWow64extLib -Force
                Write-Host "wow64ext 라이브러리 복사 완료" -ForegroundColor Green
            }
        }
    }
    Set-Location $rootDir
}

Write-Host "=== 현대적 종속성 빌드 완료 ===" -ForegroundColor Green
Write-Host "라이브러리 위치: $libDir" -ForegroundColor Cyan
Write-Host "헤더 위치: $(Join-Path $depsDir 'include')" -ForegroundColor Cyan

# 빌드 결과 요약
Write-Host "`n=== 빌드 결과 요약 ===" -ForegroundColor Magenta
Get-ChildItem $libDir -Filter "*.lib" | ForEach-Object {
    Write-Host "  ✓ $($_.Name)" -ForegroundColor Green
}

# 다음 단계 안내
Write-Host "`n=== 다음 단계 ===" -ForegroundColor Yellow
Write-Host "1. Visual Studio에서 프로젝트를 열고 다음 설정을 확인하세요:" -ForegroundColor White
Write-Host "   - 추가 포함 디렉터리: $(Join-Path $depsDir 'include')" -ForegroundColor Gray
Write-Host "   - 추가 라이브러리 디렉터리: $libDir" -ForegroundColor Gray
Write-Host "2. vcpkg 통합이 활성화되어 있는지 확인하세요" -ForegroundColor White
Write-Host "3. MacType 프로젝트를 빌드하세요" -ForegroundColor White
