# MacType 종속성 빌드 스크립트
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("x86", "x64")]
    [string]$Platform,
    
    [Parameter(Mandatory=$false)]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host "=== MacType 종속성 빌드 시작 (플랫폼: $Platform, 구성: $Configuration) ===" -ForegroundColor Green

# 디렉터리 생성
$rootDir = Get-Location
$libDir = Join-Path $rootDir "lib"
$depsDir = Join-Path $rootDir "deps"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $depsDir | Out-Null

# 1. FreeType 빌드
Write-Host "FreeType 빌드 중..." -ForegroundColor Yellow
$freetypeDir = Join-Path $depsDir "freetype-$Platform"

if (!(Test-Path $freetypeDir)) {
    Write-Host "FreeType 소스 다운로드 중..."
    git clone https://github.com/snowie2000/freetype.git $freetypeDir
}

Set-Location $freetypeDir

# 패치 적용
$patchFile = Join-Path $rootDir "doc/glyph_to_bitmapex.diff"
if (Test-Path $patchFile) {
    Write-Host "FreeType 패치 적용 중..."
    git apply $patchFile -v
}

# FreeType 빌드
$freetypeSln = "builds/windows/vc2010/freetype.sln"
if ($Platform -eq "x86") {
    msbuild $freetypeSln -p:Configuration=$Configuration -p:Platform=Win32 -v:minimal
    $sourceLib = "objs/Win32/$Configuration/freetype.lib"
    $targetLib = Join-Path $libDir "freetype.lib"
} else {
    msbuild $freetypeSln -p:Configuration=$Configuration -p:Platform=x64 -v:minimal
    $sourceLib = "objs/x64/$Configuration/freetype.lib"
    $targetLib = Join-Path $libDir "freetype64.lib"
}

if (Test-Path $sourceLib) {
    Copy-Item $sourceLib $targetLib -Force
    Write-Host "FreeType 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
} else {
    throw "FreeType 빌드 실패: $sourceLib을 찾을 수 없습니다"
}

# 환경 변수 설정
$env:FREETYPE_PATH = $freetypeDir
Write-Host "FREETYPE_PATH 설정: $freetypeDir"

Set-Location $rootDir

# 2. IniParser 빌드
Write-Host "IniParser 빌드 중..." -ForegroundColor Yellow
$iniparserDir = Join-Path $depsDir "iniparser"

if (!(Test-Path $iniparserDir)) {
    Write-Host "IniParser 소스 다운로드 중..."
    git clone https://github.com/snowie2000/IniParser.git $iniparserDir
}

Set-Location $iniparserDir

$iniparserSln = "src/IniParser.sln"
if ($Platform -eq "x86") {
    msbuild $iniparserSln -p:Configuration=$Configuration -p:Platform=x86 -v:minimal
    $sourceLib = "src/IniParser/bin/x86/$Configuration/iniparser.lib"
    $targetLib = Join-Path $libDir "iniparser.lib"
} else {
    msbuild $iniparserSln -p:Configuration=$Configuration -p:Platform=x64 -v:minimal
    $sourceLib = "src/IniParser/bin/x64/$Configuration/iniparser.lib"
    $targetLib = Join-Path $libDir "iniparser64.lib"
}

if (Test-Path $sourceLib) {
    Copy-Item $sourceLib $targetLib -Force
    Write-Host "IniParser 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
} else {
    throw "IniParser 빌드 실패: $sourceLib을 찾을 수 없습니다"
}

# 환경 변수 설정
$env:INI_PARSER_PATH = $iniparserDir
Write-Host "INI_PARSER_PATH 설정: $iniparserDir"

Set-Location $rootDir

# 3. wow64ext 빌드 (x86만)
if ($Platform -eq "x86") {
    Write-Host "wow64ext 빌드 중..." -ForegroundColor Yellow
    $wow64extDir = Join-Path $depsDir "wow64ext"
    
    if (!(Test-Path $wow64extDir)) {
        Write-Host "wow64ext 소스 다운로드 중..."
        git clone https://github.com/snowie2000/rewolf-wow64ext.git $wow64extDir
    }
    
    Set-Location $wow64extDir
    
    msbuild wow64ext.sln -p:Configuration=$Configuration -p:Platform=Win32 -v:minimal
    
    $sourceLib = "$Configuration/wow64ext.lib"
    $targetLib = Join-Path $libDir "wow64ext.lib"
    
    if (Test-Path $sourceLib) {
        Copy-Item $sourceLib $targetLib -Force
        Write-Host "wow64ext 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
    } else {
        throw "wow64ext 빌드 실패: $sourceLib을 찾을 수 없습니다"
    }
    
    Set-Location $rootDir
}

# 4. Detours 빌드
Write-Host "Detours 빌드 중..." -ForegroundColor Yellow
$detoursDir = Join-Path $depsDir "detours"

if (!(Test-Path $detoursDir)) {
    Write-Host "Detours 소스 다운로드 중..."
    git clone https://github.com/microsoft/Detours.git $detoursDir
}

Set-Location $detoursDir

# Detours는 nmake 사용
nmake

if ($Platform -eq "x86") {
    $sourceLib = "lib.X86/detours.lib"
    $targetLib = Join-Path $libDir "detours.lib"
} else {
    $sourceLib = "lib.X64/detours.lib"
    $targetLib = Join-Path $libDir "detours64.lib"
}

if (Test-Path $sourceLib) {
    Copy-Item $sourceLib $targetLib -Force
    Write-Host "Detours 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
} else {
    throw "Detours 빌드 실패: $sourceLib을 찾을 수 없습니다"
}

Set-Location $rootDir

Write-Host "=== 모든 종속성 빌드 완료 ===" -ForegroundColor Green

# 빌드된 라이브러리 목록 출력
Write-Host "`n빌드된 라이브러리:" -ForegroundColor Cyan
Get-ChildItem $libDir -Filter "*.lib" | ForEach-Object {
    Write-Host "  - $($_.Name)" -ForegroundColor White
}
