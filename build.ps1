# OW Cat 输入法构建脚本
# 使用方法: .\build.ps1 [Debug|Release] [clean]

param(
    [string]$BuildType = "Release",
    [switch]$Clean
)

# 设置错误处理
$ErrorActionPreference = "Stop"

# 获取脚本目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

Write-Host "=== OW Cat 输入法构建脚本 ===" -ForegroundColor Green
Write-Host "构建类型: $BuildType" -ForegroundColor Yellow

# 检查vcpkg环境
if (-not $env:VCPKG_ROOT) {
    Write-Host "警告: VCPKG_ROOT 环境变量未设置" -ForegroundColor Yellow
    Write-Host "请设置 VCPKG_ROOT 环境变量指向vcpkg安装目录" -ForegroundColor Yellow
    $vcpkgPath = Read-Host "请输入vcpkg安装路径（或按Enter跳过）"
    if ($vcpkgPath) {
        $env:VCPKG_ROOT = $vcpkgPath
    }
}

# 创建构建目录
$BuildDir = "build"
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "清理构建目录..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Set-Location $BuildDir

try {
    # 配置CMake
    Write-Host "配置CMake..." -ForegroundColor Yellow
    $cmakeArgs = @(
        "..",
        "-DCMAKE_BUILD_TYPE=$BuildType"
    )
    
    if ($env:VCPKG_ROOT) {
        $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
    }
    
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake配置失败"
    }
    
    # 构建项目
    Write-Host "构建项目..." -ForegroundColor Yellow
    & cmake --build . --config $BuildType --parallel
    if ($LASTEXITCODE -ne 0) {
        throw "项目构建失败"
    }
    
    Write-Host "构建成功完成!" -ForegroundColor Green
    Write-Host "可执行文件位置: $BuildDir\bin\ow_cat.exe" -ForegroundColor Green
    
} catch {
    Write-Host "构建失败: $_" -ForegroundColor Red
    exit 1
} finally {
    Set-Location $ScriptDir
}

# 显示构建结果
if (Test-Path "$BuildDir\bin\ow_cat.exe") {
    $fileInfo = Get-Item "$BuildDir\bin\ow_cat.exe"
    Write-Host "" -ForegroundColor White
    Write-Host "构建信息:" -ForegroundColor Cyan
    Write-Host "  文件大小: $([math]::Round($fileInfo.Length / 1MB, 2)) MB" -ForegroundColor White
    Write-Host "  修改时间: $($fileInfo.LastWriteTime)" -ForegroundColor White
    Write-Host "" -ForegroundColor White
    Write-Host "运行程序: .\build\bin\ow_cat.exe" -ForegroundColor Cyan
}