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

# FreeType 빌드 (문서에 따라 multi-thread release로 빌드)
$freetypeSln = "builds/windows/vc2010/freetype.sln"
if ($Platform -eq "x86") {
    Write-Host "FreeType x86 빌드 중 (multi-thread release)..." -ForegroundColor Yellow
    msbuild $freetypeSln -p:Configuration="Release Multithreaded" -p:Platform=Win32 -p:WindowsTargetPlatformVersion=10.0.22621.0 -v:minimal
    
    # 가능한 출력 경로들 확인
    $possiblePaths = @(
        "objs/Win32/Release Multithreaded/freetype.lib",
        "objs/Win32/Release/freetype.lib", 
        "objs/Win32/$Configuration/freetype.lib"
    )
    
    $sourceLib = $null
    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            $sourceLib = $path
            break
        }
    }
    $targetLib = Join-Path $libDir "freetype.lib"
} else {
    Write-Host "FreeType x64 빌드 중 (multi-thread release)..." -ForegroundColor Yellow
    msbuild $freetypeSln -p:Configuration="Release Multithreaded" -p:Platform=x64 -p:WindowsTargetPlatformVersion=10.0.22621.0 -v:minimal
    
    $possiblePaths = @(
        "objs/x64/Release Multithreaded/freetype.lib",
        "objs/x64/Release/freetype.lib",
        "objs/x64/$Configuration/freetype.lib"
    )
    
    $sourceLib = $null
    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            $sourceLib = $path
            break
        }
    }
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
        # Windows SDK 버전 문제 해결을 위해 최신 SDK로 재타겟팅
        msbuild $iniparserSln -p:Configuration=$Configuration -p:Platform=x86 -p:WindowsTargetPlatformVersion=10.0.22621.0 -v:minimal
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
        msbuild $iniparserSln -p:Configuration=$Configuration -p:Platform=x64 -p:WindowsTargetPlatformVersion=10.0.22621.0 -v:minimal
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
    
    # wow64ext 프로젝트 파일 찾기
    Write-Host "wow64ext 프로젝트 구조 확인 중..."
    Get-ChildItem -Name | Write-Host
    
    $possibleProjectFiles = @(
        "wow64ext.sln",
        "wow64ext.vcxproj",
        "*.sln",
        "*.vcxproj"
    )
    
    $projectFile = $null
    foreach ($pattern in $possibleProjectFiles) {
        $files = Get-ChildItem -Filter $pattern -ErrorAction SilentlyContinue
        if ($files) {
            $projectFile = $files[0].Name
            Write-Host "프로젝트 파일 발견: $projectFile" -ForegroundColor Green
            break
        }
    }
    
    if (-not $projectFile) {
        Write-Host "wow64ext 프로젝트 파일을 찾을 수 없어 건너뜁니다" -ForegroundColor Yellow
    } else {
        try {
            msbuild $projectFile -p:Configuration=$Configuration -p:Platform=Win32 -p:WindowsTargetPlatformVersion=10.0.22621.0 -v:minimal
            
            # 가능한 출력 경로들
            $possiblePaths = @(
                "$Configuration/wow64ext.lib",
                "Release/wow64ext.lib",
                "Debug/wow64ext.lib",
                "x86/$Configuration/wow64ext.lib",
                "Win32/$Configuration/wow64ext.lib"
            )
            
            $sourceLib = $null
            foreach ($path in $possiblePaths) {
                if (Test-Path $path) {
                    $sourceLib = $path
                    break
                }
            }
            
            $targetLib = Join-Path $libDir "wow64ext.lib"
            
            if ($sourceLib) {
                Copy-Item $sourceLib $targetLib -Force
                Write-Host "wow64ext 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
            } else {
                Write-Host "wow64ext 라이브러리 파일을 찾을 수 없습니다. 생성된 파일들:" -ForegroundColor Yellow
                Get-ChildItem -Recurse -Filter "*.lib" | ForEach-Object { Write-Host "  - $($_.FullName)" }
                Write-Host "wow64ext 라이브러리를 생성할 수 없어 건너뜁니다" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "wow64ext 빌드 실패, 건너뜁니다: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }
    
    Set-Location $rootDir
}

# 4. EasyHook 빌드 (문서에 따라 EasyHook 또는 Detours 중 선택 가능)
Write-Host "EasyHook 빌드 중..." -ForegroundColor Yellow
$easyhookDir = Join-Path $depsDir "easyhook"

if (!(Test-Path $easyhookDir)) {
    Write-Host "EasyHook 소스 다운로드 중..."
    git clone https://github.com/EasyHook/EasyHook.git $easyhookDir
}

Set-Location $easyhookDir

# EasyHook 프로젝트 구조 확인
Write-Host "EasyHook 프로젝트 구조 확인 중..."
Get-ChildItem -Name | Write-Host

# EasyHookDll 프로젝트 찾기 (문서에 따르면 EasyHookDll만 필요)
$possibleProjectFiles = @(
    "EasyHook.sln",
    "EasyHookDll/EasyHookDll.vcxproj",
    "NetFX*/EasyHookDll.sln",
    "*.sln"
)

$easyhookBuilt = $false
$projectFile = $null
foreach ($pattern in $possibleProjectFiles) {
    $files = Get-ChildItem -Filter $pattern -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($files) {
        $projectFile = $files.FullName
        Write-Host "EasyHook 프로젝트 파일 발견: $projectFile" -ForegroundColor Green
        break
    }
}

if ($projectFile) {
    try {
        if ($Platform -eq "x86") {
            Write-Host "EasyHook x86 빌드 중..." -ForegroundColor Yellow
            msbuild $projectFile -p:Configuration=$Configuration -p:Platform=Win32 -p:WindowsTargetPlatformVersion=10.0.22621.0 -v:minimal
            
            # EasyHook 출력 파일 찾기 (easyhook32.lib)
            $possiblePaths = @(
                "EasyHookDll/Win32/$Configuration/EasyHookDll.lib",
                "*/Win32/$Configuration/EasyHookDll.lib",
                "Win32/$Configuration/EasyHookDll.lib",
                "$Configuration/EasyHookDll.lib"
            )
            
            $sourceLib = $null
            foreach ($path in $possiblePaths) {
                $foundFiles = Get-ChildItem -Path $path -Recurse -ErrorAction SilentlyContinue
                if ($foundFiles) {
                    $sourceLib = $foundFiles[0].FullName
                    break
                }
            }
            
            $targetLib = Join-Path $libDir "easyhook32.lib"
        } else {
            Write-Host "EasyHook x64 빌드 중..." -ForegroundColor Yellow
            msbuild $projectFile -p:Configuration=$Configuration -p:Platform=x64 -p:WindowsTargetPlatformVersion=10.0.22621.0 -v:minimal
            
            $possiblePaths = @(
                "EasyHookDll/x64/$Configuration/EasyHookDll.lib",
                "*/x64/$Configuration/EasyHookDll.lib", 
                "x64/$Configuration/EasyHookDll.lib",
                "$Configuration/EasyHookDll.lib"
            )
            
            $sourceLib = $null
            foreach ($path in $possiblePaths) {
                $foundFiles = Get-ChildItem -Path $path -Recurse -ErrorAction SilentlyContinue
                if ($foundFiles) {
                    $sourceLib = $foundFiles[0].FullName
                    break
                }
            }
            
            $targetLib = Join-Path $libDir "easyhook64.lib"
        }
        
        if ($sourceLib) {
            Copy-Item $sourceLib $targetLib -Force
            Write-Host "EasyHook 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
            $easyhookBuilt = $true
        } else {
            Write-Host "EasyHook 라이브러리 파일을 찾을 수 없습니다. 생성된 파일들:" -ForegroundColor Yellow
            Get-ChildItem -Recurse -Filter "*.lib" | ForEach-Object { Write-Host "  - $($_.FullName)" }
            Write-Host "EasyHook 대신 Detours를 사용합니다" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "EasyHook 빌드 실패, Detours로 대체: $($_.Exception.Message)" -ForegroundColor Yellow
    }
} else {
    Write-Host "EasyHook 프로젝트 파일을 찾을 수 없어 Detours를 사용합니다" -ForegroundColor Yellow
}

Set-Location $rootDir

# 5. Detours 빌드 (EasyHook 대체용 또는 보완용)
Write-Host "Detours 빌드 중..." -ForegroundColor Yellow
$detoursDir = Join-Path $depsDir "detours"

if (!(Test-Path $detoursDir)) {
    Write-Host "Detours 소스 다운로드 중..."
    git clone https://github.com/microsoft/Detours.git $detoursDir
}

Set-Location $detoursDir

# Detours는 nmake 사용 - Visual Studio Command Prompt 환경에서 실행
try {
    # Visual Studio의 vcvars 스크립트 실행 후 nmake
    cmd /c "call `"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat`" && nmake"
} catch {
    Write-Host "vcvars32.bat를 사용한 빌드 실패, 직접 nmake 시도..." -ForegroundColor Yellow
    nmake
}

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
    Write-Host "Detours 라이브러리 파일을 찾을 수 없습니다. 생성된 파일들:" -ForegroundColor Yellow
    Get-ChildItem -Recurse -Filter "*.lib" | ForEach-Object { Write-Host "  - $($_.FullName)" }
    
    if (-not $easyhookBuilt) {
        Write-Host "EasyHook와 Detours 모두 빌드 실패했습니다. 후킹 라이브러리가 없으면 MacType 빌드가 실패할 수 있습니다." -ForegroundColor Red
    } else {
        Write-Host "EasyHook가 성공적으로 빌드되었으므로 Detours 없이도 진행 가능합니다." -ForegroundColor Yellow
    }
}

Set-Location $rootDir

Write-Host "=== 모든 종속성 빌드 완료 ===" -ForegroundColor Green

# 빌드된 라이브러리 목록 출력 및 HOWTOBUILD.md 요구사항 확인
Write-Host "`n빌드된 라이브러리:" -ForegroundColor Cyan
$builtLibs = Get-ChildItem $libDir -Filter "*.lib" | ForEach-Object { $_.Name }
$builtLibs | ForEach-Object {
    Write-Host "  ✓ $_" -ForegroundColor Green
}

# HOWTOBUILD.md 문서에 따른 필수 라이브러리 확인
Write-Host "`nHOWTOBUILD.md 요구사항 확인:" -ForegroundColor Cyan

$requiredLibs = @()
if ($Platform -eq "x86") {
    $requiredLibs = @(
        "freetype.lib",       # FreeType (필수)
        "iniparser.lib",      # IniParser (필수) 
        "wow64ext.lib"        # wow64ext (x86만 필요)
    )
    # EasyHook 또는 Detours 중 하나 (필수)
    $hookingLib = @("easyhook32.lib", "detours.lib")
} else {
    $requiredLibs = @(
        "freetype64.lib",     # FreeType (필수)
        "iniparser64.lib"     # IniParser (필수)
    )
    # EasyHook 또는 Detours 중 하나 (필수) 
    $hookingLib = @("easyhook64.lib", "detours64.lib")
}

# 필수 라이브러리 확인
foreach ($lib in $requiredLibs) {
    if ($lib -in $builtLibs) {
        Write-Host "  ✓ $lib (필수)" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $lib (필수 - 누락!)" -ForegroundColor Red
    }
}

# 후킹 라이브러리 확인 (EasyHook 또는 Detours 중 하나)
$hookingFound = $false
foreach ($lib in $hookingLib) {
    if ($lib -in $builtLibs) {
        Write-Host "  ✓ $lib (후킹 라이브러리)" -ForegroundColor Green
        $hookingFound = $true
        break
    }
}

if (-not $hookingFound) {
    Write-Host "  ✗ 후킹 라이브러리 누락! (easyhook 또는 detours 필요)" -ForegroundColor Red
}

# 환경 변수 확인
Write-Host "`n환경 변수 설정 확인:" -ForegroundColor Cyan
if ($env:FREETYPE_PATH) {
    Write-Host "  ✓ FREETYPE_PATH: $env:FREETYPE_PATH" -ForegroundColor Green
} else {
    Write-Host "  ✗ FREETYPE_PATH 미설정" -ForegroundColor Red
}

if ($env:INI_PARSER_PATH) {
    Write-Host "  ✓ INI_PARSER_PATH: $env:INI_PARSER_PATH" -ForegroundColor Green
} else {
    Write-Host "  ✗ INI_PARSER_PATH 미설정" -ForegroundColor Red
}

Write-Host "`n다음 단계: 모든 라이브러리를 lib/ 폴더에 배치한 후 MacType을 빌드하세요" -ForegroundColor Cyan
Write-Host "빌드 명령: msbuild gdipp.sln -p:Configuration=$Configuration -p:Platform=$(if ($Platform -eq 'x86') { 'Win32' } else { 'x64' })" -ForegroundColor White
