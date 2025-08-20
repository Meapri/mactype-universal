param(
  [Parameter(Mandatory=$true)]
  [string]$AssetUrl,

  [Parameter(Mandatory=$false)]
  [string]$OutputDir = "installer\upstream"
)

$ErrorActionPreference = "Stop"

Write-Host "=== Upstream asset 다운로드 및 추출 ===" -ForegroundColor Green
Write-Host "URL: $AssetUrl" -ForegroundColor Cyan

$root = (Get-Location)
$tmp = Join-Path $env:RUNNER_TEMP "mactype_upstream"
if (-not $tmp) { $tmp = Join-Path $root ".tmp_upstream" }
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$fileName = Split-Path $AssetUrl -Leaf
$downloadPath = Join-Path $tmp $fileName

Write-Host "다운로드 중... -> $downloadPath"
Invoke-WebRequest -Uri $AssetUrl -OutFile $downloadPath -UseBasicParsing

function Ensure-7zip {
  $sevenZip = (Get-Command 7z -ErrorAction SilentlyContinue)
  if (-not $sevenZip) {
    Write-Host "7-Zip 미설치: Chocolatey로 설치 시도" -ForegroundColor Yellow
    try { choco install 7zip -y --no-progress | Out-Null } catch {}
  }
  return (Get-Command 7z -ErrorAction SilentlyContinue)
}

$isZip = $fileName.ToLower().EndsWith('.zip')
$isExe = $fileName.ToLower().EndsWith('.exe')

if ($isZip) {
  Write-Host "ZIP 추출" -ForegroundColor Cyan
  Expand-Archive -Path $downloadPath -DestinationPath $OutputDir -Force
} elseif ($isExe) {
  $seven = Ensure-7zip
  if (-not $seven) { throw "7-Zip가 필요합니다 (exe 추출)" }
  Write-Host "Installer(EXE) 7-Zip으로 추출" -ForegroundColor Cyan
  & 7z x "$downloadPath" -o"$OutputDir" -y | Out-Null
} else {
  throw "지원하지 않는 파일 형식: $fileName"
}

Write-Host "=== .ini 및 프로필 수집 ===" -ForegroundColor Green
$destConfig = Join-Path $OutputDir "config"
New-Item -ItemType Directory -Force -Path $destConfig | Out-Null

$iniFiles = Get-ChildItem -Path $OutputDir -Recurse -Include *.ini -ErrorAction SilentlyContinue
foreach ($f in $iniFiles) {
  $rel = $f.FullName.Substring($OutputDir.Length).TrimStart('\\','/')
  Write-Host "복사: $rel" -ForegroundColor Gray
  Copy-Item $f.FullName -Destination $destConfig -Force
}

Write-Host "완료. 출력 경로: $OutputDir" -ForegroundColor Green


