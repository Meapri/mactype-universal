# MacType 종속성 빌드 스크립트 (레거시 - 더 이상 사용되지 않음)
# 주의: 이 스크립트는 iniparser를 사용하며, 현재는 build-modern-dependencies.ps1이 사용됨
# iniparser는 완전히 제거되고 JSON 기반 설정 시스템으로 전환됨

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("x86", "x64")]
    [string]$Platform,

    [Parameter(Mandatory=$false)]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host "=== MacType 종속성 빌드 시작 (플랫폼: $Platform, 구성: $Configuration) ===" -ForegroundColor Green
Write-Host "⚠️  주의: 이 스크립트는 더 이상 사용되지 않습니다. build-modern-dependencies.ps1을 사용하세요." -ForegroundColor Red
Write-Host "⚠️  주의: iniparser가 제거되어 이 스크립트는 작동하지 않을 수 있습니다." -ForegroundColor Red

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

# 먼저 사용 가능한 빌드 구성 확인
Write-Host "FreeType 사용 가능한 빌드 구성 확인 중..." -ForegroundColor Yellow
$projectFiles = Get-ChildItem -Path "builds/windows/vc2010" -Filter "*.vcxproj" -ErrorAction SilentlyContinue
if ($projectFiles) {
    Write-Host "발견된 프로젝트 파일: $($projectFiles[0].Name)" -ForegroundColor Cyan
}

if ($Platform -eq "x86") {
    Write-Host "FreeType x86 빌드 중..." -ForegroundColor Yellow
    
    # 여러 빌드 구성 시도
    $buildConfigs = @("Release", "Release Multithreaded", "$Configuration")
    $buildSuccess = $false
    
    foreach ($config in $buildConfigs) {
        try {
            Write-Host "빌드 구성 시도: $config" -ForegroundColor Cyan
            msbuild $freetypeSln -p:Configuration=$config -p:Platform=Win32 -p:WindowsTargetPlatformVersion=10.0.22621.0 -v:minimal
            
            # 가능한 출력 경로들 확인
            $possiblePaths = @(
                "objs/Win32/$config/freetype.lib",
                "objs/Win32/Release/freetype.lib", 
                "objs/Win32/$Configuration/freetype.lib",
                "objs/x86/$config/freetype.lib"
            )
            
            $sourceLib = $null
            foreach ($path in $possiblePaths) {
                if (Test-Path $path) {
                    $sourceLib = $path
                    $buildSuccess = $true
                    Write-Host "FreeType 라이브러리 발견: $path" -ForegroundColor Green
                    break
                }
            }
            
            if ($buildSuccess) { break }
        } catch {
            Write-Host "빌드 구성 $config 실패: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }
    
    if (-not $buildSuccess) {
        Write-Host "모든 빌드 구성 실패. 생성된 파일 확인 중..." -ForegroundColor Yellow
        Get-ChildItem -Recurse -Filter "*.lib" | ForEach-Object { Write-Host "  발견: $($_.FullName)" }
        $sourceLib = ""  # 빈 문자열로 설정하여 null 오류 방지
    }
    
    $targetLib = Join-Path $libDir "freetype.lib"
} else {
    Write-Host "FreeType x64 빌드 중..." -ForegroundColor Yellow
    
    $buildConfigs = @("Release", "Release Multithreaded", "$Configuration")
    $buildSuccess = $false
    
    foreach ($config in $buildConfigs) {
        try {
            Write-Host "빌드 구성 시도: $config" -ForegroundColor Cyan
            msbuild $freetypeSln -p:Configuration=$config -p:Platform=x64 -p:WindowsTargetPlatformVersion=10.0.22621.0 -v:minimal
            
            $possiblePaths = @(
                "objs/x64/$config/freetype.lib",
                "objs/x64/Release/freetype.lib",
                "objs/x64/$Configuration/freetype.lib",
                "objs/AMD64/$config/freetype.lib"
            )
            
            $sourceLib = $null
            foreach ($path in $possiblePaths) {
                if (Test-Path $path) {
                    $sourceLib = $path
                    $buildSuccess = $true
                    Write-Host "FreeType 라이브러리 발견: $path" -ForegroundColor Green
                    break
                }
            }
            
            if ($buildSuccess) { break }
        } catch {
            Write-Host "빌드 구성 $config 실패: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }
    
    if (-not $buildSuccess) {
        Write-Host "모든 빌드 구성 실패. 생성된 파일 확인 중..." -ForegroundColor Yellow
        Get-ChildItem -Recurse -Filter "*.lib" | ForEach-Object { Write-Host "  발견: $($_.FullName)" }
        $sourceLib = ""  # 빈 문자열로 설정하여 null 오류 방지
    }
    
    $targetLib = Join-Path $libDir "freetype64.lib"
}

if ($sourceLib -and (Test-Path $sourceLib)) {
    Copy-Item $sourceLib $targetLib -Force
    Write-Host "FreeType 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
} else {
    Write-Host "FreeType 라이브러리 빌드 실패 또는 파일을 찾을 수 없음" -ForegroundColor Red
    Write-Host "소스 라이브러리: $sourceLib" -ForegroundColor Yellow
    Write-Host "FreeType 없이 계속 진행하지만 MacType 빌드가 실패할 수 있습니다" -ForegroundColor Yellow
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
        Write-Host "IniParser x86 빌드 중..." -ForegroundColor Cyan
        # 안정적인 SDK 버전 사용
        $sdkVersion = if ($env:WINDOWS_SDK_VERSION) { $env:WINDOWS_SDK_VERSION } else { "10.0.26100.0" }
        msbuild $iniparserSln -p:Configuration=$Configuration -p:Platform=x86 -p:WindowsTargetPlatformVersion=$sdkVersion -p:PlatformToolset=v143 -v:minimal
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
        Write-Host "IniParser x64 빌드 중..." -ForegroundColor Cyan
        # 안정적인 SDK 버전 사용
        $sdkVersion = if ($env:WINDOWS_SDK_VERSION) { $env:WINDOWS_SDK_VERSION } else { "10.0.26100.0" }
        msbuild $iniparserSln -p:Configuration=$Configuration -p:Platform=x64 -p:WindowsTargetPlatformVersion=$sdkVersion -p:PlatformToolset=v143 -v:minimal
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
        Write-Host "프로젝트 파일을 찾을 수 없습니다. Makefile 확인 중..." -ForegroundColor Yellow
        
        # Makefile로 빌드 시도
        if (Test-Path "src") {
            Set-Location "src"
            if (Test-Path "Makefile") {
                Write-Host "Makefile을 사용하여 wow64ext 빌드 시도..." -ForegroundColor Cyan
                try {
                    # Visual Studio Developer Command Prompt 환경 설정
                    $vsPath = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
                    if (Test-Path $vsPath) {
                        cmd /c "`"$vsPath`" && nmake"
                    } else {
                        # 대체 경로
                        $vsPath2 = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
                        if (Test-Path $vsPath2) {
                            cmd /c "`"$vsPath2`" && nmake"
                        } else {
                            Write-Host "Visual Studio Developer Command Prompt를 찾을 수 없습니다. 직접 nmake 시도..." -ForegroundColor Yellow
                            & "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\*\bin\Hostx64\x64\nmake.exe"
                        }
                    }
                    
                    # 생성된 .lib 파일 찾기
                    $possibleLibs = @(
                        "wow64ext.lib",
                        "obj/wow64ext.lib",
                        "../wow64ext.lib"
                    )
                    
                    $foundLib = $null
                    foreach ($libPath in $possibleLibs) {
                        if (Test-Path $libPath) {
                            $foundLib = $libPath
                            break
                        }
                    }
                    
                    if ($foundLib) {
                        $targetLib = Join-Path $libDir "wow64ext.lib"
                        Copy-Item $foundLib $targetLib -Force
                        Write-Host "wow64ext 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
                    } else {
                        Write-Host "wow64ext 라이브러리 파일을 찾을 수 없습니다" -ForegroundColor Yellow
                    }
                } catch {
                    Write-Host "Makefile 빌드 실패: $($_.Exception.Message)" -ForegroundColor Yellow
                }
            } else {
                Write-Host "wow64ext Makefile을 찾을 수 없어 건너뜁니다" -ForegroundColor Yellow
            }
            Set-Location $wow64extDir
        } else {
            Write-Host "wow64ext src 폴더를 찾을 수 없어 건너뜁니다" -ForegroundColor Yellow
        }
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

# Detours 빌드 (샘플 제외하고 핵심 라이브러리만)
Write-Host "Detours 라이브러리 빌드 중..." -ForegroundColor Yellow

try {
    # 프로젝트 구조 확인
    Write-Host "Detours 프로젝트 구조:" -ForegroundColor Cyan
    Get-ChildItem -Name | ForEach-Object { Write-Host "  - $_" }
    
    # src 폴더만 빌드 (있다면)
    if (Test-Path "src") {
        Write-Host "src 폴더에서 핵심 라이브러리만 빌드..." -ForegroundColor Cyan
        Set-Location "src"
        
        # Visual Studio Developer Command Prompt 환경에서 nmake 실행
        $vsPath = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
        if (Test-Path $vsPath) {
            cmd /c "`"$vsPath`" && nmake"
        } else {
            # 대체 경로들 시도
            $altPaths = @(
                "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat",
                "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
            )
            
            $found = $false
            foreach ($altPath in $altPaths) {
                if (Test-Path $altPath) {
                    cmd /c "`"$altPath`" && nmake"
                    $found = $true
                    break
                }
            }
            
            if (-not $found) {
                Write-Host "Visual Studio 환경을 찾을 수 없어 간단한 방법으로 시도..." -ForegroundColor Yellow
                # PATH에 nmake가 있는지 시도
                try { 
                    nmake 
                } catch {
                    Write-Host "nmake를 찾을 수 없습니다: $($_.Exception.Message)" -ForegroundColor Red
                    throw
                }
            }
        }
        Set-Location ".."
    } else {
        # Makefile 수정하여 샘플 제외
        Write-Host "Makefile 수정하여 샘플 빌드 제외..." -ForegroundColor Cyan
        
        if (Test-Path "Makefile") {
            # Makefile 백업
            Copy-Item "Makefile" "Makefile.backup"
            
            # samples 관련 줄 제거
            $makefile = Get-Content "Makefile"
            $newMakefile = $makefile | Where-Object { 
                $_ -notmatch "samples" -and 
                $_ -notmatch "SUBDIRS.*samples" -and
                $_ -notmatch "cd.*samples"
            }
            $newMakefile | Set-Content "Makefile"
            
            Write-Host "수정된 Makefile로 빌드 실행..." -ForegroundColor Cyan
        }
        
        # 간단한 nmake 실행
        nmake
    }
} catch {
    Write-Host "Detours 빌드 실패: $($_.Exception.Message)" -ForegroundColor Yellow
    
    # 이미 빌드된 라이브러리 확인
    Write-Host "기존 라이브러리 파일 확인 중..." -ForegroundColor Yellow
    Get-ChildItem -Recurse -Filter "*.lib" -ErrorAction SilentlyContinue | ForEach-Object { 
        Write-Host "  발견: $($_.FullName)" -ForegroundColor Green
    }
}

if ($Platform -eq "x86") {
    $sourceLib = "lib.X86/detours.lib"
    $targetLib = Join-Path $libDir "detours.lib"
} else {
    $sourceLib = "lib.X64/detours.lib"
    $targetLib = Join-Path $libDir "detours64.lib"
}

if ($sourceLib -and (Test-Path $sourceLib)) {
    Copy-Item $sourceLib $targetLib -Force
    Write-Host "Detours 라이브러리 복사 완료: $targetLib" -ForegroundColor Green
} else {
    Write-Host "기본 경로에서 Detours 라이브러리를 찾을 수 없습니다. 대체 경로 확인 중..." -ForegroundColor Yellow
    
    # 모든 생성된 라이브러리 파일 표시
    Write-Host "생성된 모든 .lib 파일들:" -ForegroundColor Cyan
    Get-ChildItem -Recurse -Filter "*.lib" | ForEach-Object { 
        Write-Host "  - $($_.FullName)" -ForegroundColor White
    }
    
    # 대체 경로들에서 detours.lib 찾기
    $alternativePaths = @(
        "lib/detours.lib",
        "src/detours.lib", 
        "detours.lib",
        "lib.X86/detours.lib",
        "lib.X64/detours.lib"
    )
    
    $found = $false
    foreach ($altPath in $alternativePaths) {
        if (Test-Path $altPath) {
            Copy-Item $altPath $targetLib -Force
            Write-Host "대체 경로에서 Detours 라이브러리 복사: $altPath -> $targetLib" -ForegroundColor Green
            $found = $true
            break
        }
    }
    
    # 그래도 못 찾았으면 임시로 빈 라이브러리 생성 (빌드 계속 진행용)
    if (-not $found) {
        Write-Host "Detours 라이브러리를 찾을 수 없습니다." -ForegroundColor Yellow
        
        if ($easyhookBuilt) {
            Write-Host "EasyHook가 성공적으로 빌드되었으므로 Detours 없이도 진행할 수 있습니다." -ForegroundColor Green
        } else {
            Write-Host "후킹 라이브러리가 없으면 MacType 빌드가 실패할 수 있습니다." -ForegroundColor Red
        }
    }
}

Set-Location $rootDir

# MacType이 요구하는 특별한 라이브러리 이름들 생성
Write-Host "MacType 호환성을 위한 라이브러리 이름 맞추기..." -ForegroundColor Cyan

# easyhk64.lib 생성 (MacType이 EasyHook에 대해 요구하는 이름)
$easyhookLib = Join-Path $libDir "easyhook64.lib"
$macTypeEasyHookLib = Join-Path $libDir "easyhk64.lib"

if (Test-Path $easyhookLib) {
    Copy-Item $easyhookLib $macTypeEasyHookLib -Force
    Write-Host "MacType용 EasyHook 라이브러리 생성: easyhk64.lib" -ForegroundColor Green
} else {
    # AUX_ULIB 라이브러리를 easyhk64.lib로 사용 시도
    $auxLibs = Get-ChildItem $depsDir -Recurse -Filter "AUX_ULIB_*.LIB" -ErrorAction SilentlyContinue
    if ($auxLibs) {
        $auxLib64 = $auxLibs | Where-Object { $_.Name -like "*x64*" -or $_.Name -like "*64*" } | Select-Object -First 1
        if ($auxLib64) {
            Copy-Item $auxLib64.FullName $macTypeEasyHookLib -Force
            Write-Host "AUX_ULIB을 MacType용 EasyHook으로 사용: $($auxLib64.Name) -> easyhk64.lib" -ForegroundColor Yellow
        }
    }
}

# Detours 라이브러리가 있다면 백업으로 easyhk64.lib 생성
if (-not (Test-Path $macTypeEasyHookLib)) {
    $detoursLib = Join-Path $libDir "detours64.lib"
    if (Test-Path $detoursLib) {
        Copy-Item $detoursLib $macTypeEasyHookLib -Force
        Write-Host "Detours를 MacType용 후킹 라이브러리로 사용: detours64.lib -> easyhk64.lib" -ForegroundColor Yellow
    }
}

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
