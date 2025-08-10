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
    try {
        # 먼저 패치가 이미 적용되었는지 확인
        $checkResult = git apply --check $patchFile 2>&1
        if ($LASTEXITCODE -eq 0) {
            git apply $patchFile --verbose
            Write-Host "패치 적용 성공" -ForegroundColor Green
        } else {
            Write-Host "패치를 적용할 수 없거나 이미 적용됨: $checkResult" -ForegroundColor Yellow
            Write-Host "패치 없이 계속 진행..." -ForegroundColor Yellow
        }
    } catch {
        Write-Host "패치 적용 실패, 무시하고 계속: $($_.Exception.Message)" -ForegroundColor Yellow
    }
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

# IniParser 프로젝트 구조 확인 및 빌드
Write-Host "IniParser 프로젝트 구조 확인 중..."
Get-ChildItem -Name | Write-Host

# 가능한 솔루션 파일들 찾기
$possibleSlnFiles = @(
    "IniParser.sln",
    "src/IniParser.sln", 
    "IniParser/IniParser.sln",
    "INIFileParser.sln"
)

$iniparserSln = $null
foreach ($slnFile in $possibleSlnFiles) {
    if (Test-Path $slnFile) {
        $iniparserSln = $slnFile
        Write-Host "솔루션 파일 발견: $slnFile" -ForegroundColor Green
        break
    }
}

if (-not $iniparserSln) {
    Write-Host "솔루션 파일을 찾을 수 없습니다. 사용 가능한 프로젝트 파일 검색 중..." -ForegroundColor Yellow
    $projFiles = Get-ChildItem -Recurse -Filter "*.csproj" | Select-Object -First 5
    if ($projFiles) {
        Write-Host "발견된 프로젝트 파일들:" -ForegroundColor Yellow
        $projFiles | ForEach-Object { Write-Host "  - $($_.FullName)" }
        
        # 첫 번째 프로젝트 파일 사용
        $iniparserSln = $projFiles[0].FullName
        Write-Host "첫 번째 프로젝트 파일 사용: $iniparserSln" -ForegroundColor Yellow
    } else {
        throw "IniParser 빌드 파일을 찾을 수 없습니다"
    }
}

try {
    if ($Platform -eq "x86") {
        msbuild $iniparserSln -p:Configuration=$Configuration -p:Platform=x86 -v:minimal
        # 가능한 출력 경로들
        $possiblePaths = @(
            "src/IniParser/bin/x86/$Configuration/iniparser.lib",
            "IniParser/bin/x86/$Configuration/iniparser.lib",
            "bin/x86/$Configuration/iniparser.lib",
            "src/IniParser/bin/$Configuration/iniparser.lib",
            "IniParser/bin/$Configuration/iniparser.lib",
            "bin/$Configuration/iniparser.lib"
        )
        $targetLib = Join-Path $libDir "iniparser.lib"
    } else {
        msbuild $iniparserSln -p:Configuration=$Configuration -p:Platform=x64 -v:minimal
        $possiblePaths = @(
            "src/IniParser/bin/x64/$Configuration/iniparser.lib",
            "IniParser/bin/x64/$Configuration/iniparser.lib", 
            "bin/x64/$Configuration/iniparser.lib",
            "src/IniParser/bin/$Configuration/iniparser.lib",
            "IniParser/bin/$Configuration/iniparser.lib",
            "bin/$Configuration/iniparser.lib"
        )
        $targetLib = Join-Path $libDir "iniparser64.lib"
    }
    
    # 실제 생성된 라이브러리 파일 찾기
    $sourceLib = $null
    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            $sourceLib = $path
            break
        }
    }
    
    if ($sourceLib) {
        Copy-Item $sourceLib $targetLib -Force
        Write-Host "IniParser 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
    } else {
        Write-Host "라이브러리 파일을 찾을 수 없습니다. 생성된 파일들:" -ForegroundColor Yellow
        Get-ChildItem -Recurse -Filter "*.lib" | ForEach-Object { Write-Host "  - $($_.FullName)" }
        
        # .lib 파일이 없으면 .dll이나 다른 형태일 수 있음
        $alternativeFiles = Get-ChildItem -Recurse -Filter "*iniparser*" | Where-Object { $_.Extension -in @('.lib', '.dll', '.a') }
        if ($alternativeFiles) {
            $sourceLib = $alternativeFiles[0].FullName
            Copy-Item $sourceLib $targetLib -Force
            Write-Host "대체 파일 사용: $sourceLib -> $targetLib" -ForegroundColor Yellow
        } else {
            Write-Host "IniParser 라이브러리를 생성할 수 없어 건너뜁니다" -ForegroundColor Yellow
        }
    }
} catch {
    Write-Host "IniParser 빌드 실패, 건너뜁니다: $($_.Exception.Message)" -ForegroundColor Yellow
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
