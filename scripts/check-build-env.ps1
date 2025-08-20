# MacType 빌드 환경 체크 스크립트
param(
    [switch]$Verbose
)

function Write-Status {
    param([string]$Message, [string]$Status, [string]$Color = "White")
    $statusText = "[$Status]".PadRight(8)
    Write-Host "$statusText $Message" -ForegroundColor $Color
}

function Test-Command {
    param([string]$Command)
    try {
        $null = Get-Command $Command -ErrorAction Stop
        return $true
    } catch {
        return $false
    }
}

function Test-MSBuildVersion {
    try {
        $msbuildOutput = msbuild -version 2>&1
        if ($msbuildOutput -match "Microsoft \(R\) Build Engine version (\d+\.\d+\.\d+\.\d+)") {
            return $matches[1]
        }
        return $null
    } catch {
        return $null
    }
}

Write-Host "=== MacType 빌드 환경 체크 ===" -ForegroundColor Cyan
Write-Host ""

# 1. 필수 도구 체크
Write-Host "필수 도구 체크:" -ForegroundColor Yellow

if (Test-Command "msbuild") {
    $version = Test-MSBuildVersion
    if ($version) {
        Write-Status "MSBuild" "OK" "Green"
        if ($Verbose) { Write-Host "    버전: $version" -ForegroundColor Gray }
    } else {
        Write-Status "MSBuild" "경고" "Yellow"
        Write-Host "    MSBuild가 있지만 버전을 확인할 수 없습니다" -ForegroundColor Gray
    }
} else {
    Write-Status "MSBuild" "오류" "Red"
    Write-Host "    MSBuild를 찾을 수 없습니다. Visual Studio 또는 Build Tools를 설치하세요" -ForegroundColor Gray
}

if (Test-Command "git") {
    Write-Status "Git" "OK" "Green"
    if ($Verbose) {
        $gitVersion = git --version
        Write-Host "    $gitVersion" -ForegroundColor Gray
    }
} else {
    Write-Status "Git" "오류" "Red"
    Write-Host "    Git를 찾을 수 없습니다" -ForegroundColor Gray
}

if (Test-Command "nmake") {
    Write-Status "NMAKE" "OK" "Green"
} else {
    Write-Status "NMAKE" "경고" "Yellow"
    Write-Host "    NMAKE를 찾을 수 없습니다. Detours 빌드에 필요합니다" -ForegroundColor Gray
}

Write-Host ""

# 2. 디렉터리 구조 체크
Write-Host "디렉터리 구조 체크:" -ForegroundColor Yellow

$requiredFiles = @(
    "gdipp.sln",
    "gdipp.vcxproj", 
    "doc/HOWTOBUILD.md",
    "doc/glyph_to_bitmapex.diff",
    "scripts/build-dependencies.ps1"
)

foreach ($file in $requiredFiles) {
    if (Test-Path $file) {
        Write-Status $file "OK" "Green"
    } else {
        Write-Status $file "누락" "Red"
    }
}

Write-Host ""

# 3. 라이브러리 폴더 체크
Write-Host "라이브러리 폴더 체크:" -ForegroundColor Yellow

$libDir = "lib"
if (Test-Path $libDir) {
    Write-Status "lib 폴더" "OK" "Green"
    
    $libFiles = @(
        "freetype.lib",
        "freetype64.lib",
        "wow64ext.lib",
        "detours.lib",
        "detours64.lib"
    )
    
    $foundLibs = Get-ChildItem $libDir -Filter "*.lib" | Select-Object -ExpandProperty Name
    
    if ($foundLibs.Count -eq 0) {
        Write-Status "라이브러리 파일" "없음" "Yellow"
        Write-Host "    종속성을 먼저 빌드하세요: .\scripts\build-dependencies.ps1" -ForegroundColor Gray
    } else {
        Write-Status "라이브러리 파일" "발견" "Green"
        if ($Verbose) {
            Write-Host "    발견된 라이브러리:" -ForegroundColor Gray
            foreach ($lib in $foundLibs) {
                Write-Host "      - $lib" -ForegroundColor Gray
            }
        }
        
        # 누락된 라이브러리 체크
        $missingLibs = $libFiles | Where-Object { $_ -notin $foundLibs }
        if ($missingLibs.Count -gt 0) {
            Write-Host "    누락된 라이브러리:" -ForegroundColor Yellow
            foreach ($lib in $missingLibs) {
                Write-Host "      - $lib" -ForegroundColor Yellow
            }
        }
    }
} else {
    Write-Status "lib 폴더" "없음" "Yellow"
    Write-Host "    종속성을 먼저 빌드하세요: .\scripts\build-dependencies.ps1" -ForegroundColor Gray
}

Write-Host ""

# 4. 환경 변수 체크
Write-Host "환경 변수 체크:" -ForegroundColor Yellow

$envVars = @{
    "FREETYPE_PATH" = "FreeType 소스 경로"
    "INI_PARSER_PATH" = "IniParser 소스 경로"
}

foreach ($var in $envVars.GetEnumerator()) {
    $value = [Environment]::GetEnvironmentVariable($var.Key)
    if ($value) {
        if (Test-Path $value) {
            Write-Status "$($var.Key)" "OK" "Green"
            if ($Verbose) { Write-Host "    경로: $value" -ForegroundColor Gray }
        } else {
            Write-Status "$($var.Key)" "경로 오류" "Red"
            Write-Host "    설정된 경로가 존재하지 않습니다: $value" -ForegroundColor Gray
        }
    } else {
        Write-Status "$($var.Key)" "미설정" "Yellow"
        Write-Host "    $($var.Value)" -ForegroundColor Gray
    }
}

Write-Host ""

# 5. Windows SDK 체크
Write-Host "Windows SDK 체크:" -ForegroundColor Yellow

$sdkPath = "${env:ProgramFiles(x86)}\Windows Kits\10"
if (Test-Path $sdkPath) {
    Write-Status "Windows 10 SDK" "설치됨" "Green"
    
    $includePath = Join-Path $sdkPath "Include"
    if (Test-Path $includePath) {
        $versions = Get-ChildItem $includePath -Directory | Where-Object { $_.Name -match "^\d+\.\d+\.\d+\.\d+$" } | Sort-Object Name -Descending
        if ($versions.Count -gt 0) {
            $latestVersion = $versions[0].Name
            Write-Status "최신 SDK 버전" $latestVersion "Green"
            
            # 최소 요구사항 체크 (10.0.14393.0)
            if ([version]$latestVersion -ge [version]"10.0.14393.0") {
                Write-Status "SDK 버전 요구사항" "충족" "Green"
            } else {
                Write-Status "SDK 버전 요구사항" "미충족" "Red"
                Write-Host "    최소 10.0.14393.0 이상이 필요합니다" -ForegroundColor Gray
            }
        }
    }
} else {
    Write-Status "Windows 10 SDK" "없음" "Red"
    Write-Host "    Windows 10 SDK를 설치하세요" -ForegroundColor Gray
}

Write-Host ""

# 요약
Write-Host "=== 요약 ===" -ForegroundColor Cyan

$canBuild = $true
$warnings = @()
$errors = @()

if (!(Test-Command "msbuild")) {
    $errors += "MSBuild가 설치되지 않음"
    $canBuild = $false
}

if (!(Test-Command "git")) {
    $errors += "Git가 설치되지 않음"
    $canBuild = $false
}

if (!(Test-Path "gdipp.sln")) {
    $errors += "솔루션 파일을 찾을 수 없음"
    $canBuild = $false
}

if (!(Test-Path "lib") -or (Get-ChildItem "lib" -Filter "*.lib").Count -eq 0) {
    $warnings += "종속성 라이브러리가 빌드되지 않음"
}

if (!(Test-Command "nmake")) {
    $warnings += "NMAKE가 없어 Detours 빌드가 어려울 수 있음"
}

if ($errors.Count -eq 0 -and $canBuild) {
    Write-Host "✅ 빌드 환경이 준비되었습니다!" -ForegroundColor Green
} else {
    Write-Host "❌ 빌드 환경에 문제가 있습니다:" -ForegroundColor Red
    foreach ($error in $errors) {
        Write-Host "   - $error" -ForegroundColor Red
    }
}

if ($warnings.Count -gt 0) {
    Write-Host "⚠️  경고사항:" -ForegroundColor Yellow
    foreach ($warning in $warnings) {
        Write-Host "   - $warning" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "다음 단계:" -ForegroundColor Cyan
if ($warnings -contains "종속성 라이브러리가 빌드되지 않음") {
    Write-Host "1. 종속성 빌드: .\scripts\build-dependencies.ps1 -Platform x86" -ForegroundColor White
    Write-Host "2. 종속성 빌드: .\scripts\build-dependencies.ps1 -Platform x64" -ForegroundColor White
    Write-Host "3. MacType 빌드: msbuild gdipp.sln -p:Configuration=Release -p:Platform=Win32" -ForegroundColor White
} elseif ($canBuild) {
    Write-Host "MacType 빌드: msbuild gdipp.sln -p:Configuration=Release -p:Platform=Win32" -ForegroundColor White
}
