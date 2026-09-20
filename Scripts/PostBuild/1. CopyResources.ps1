param(
    [Parameter(Mandatory = $true)]
    [string] $TargetDirectory
)

$ErrorActionPreference = "Stop"

# 스크립트의 위치를 기준으로 프로젝트 루트와 빌드 출력 폴더를 결정합니다.
$projectRootPath = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$targetDirectoryPath = [System.IO.Path]::GetFullPath($TargetDirectory)

# Resources의 원본 폴더와 출력 폴더에서 사용할 이름을 함께 정의합니다.
$resourceDirectories = @(
    "Textures"
    "Edit"
)

foreach ($resourceDirectoryName in $resourceDirectories) {
    $sourceDirectoryPath = Join-Path $projectRootPath "Resources\$resourceDirectoryName"
    $destinationDirectoryPath = Join-Path $targetDirectoryPath $resourceDirectoryName

    if (-not (Test-Path -LiteralPath $sourceDirectoryPath -PathType Container)) {
        throw "복사할 리소스 폴더를 찾을 수 없습니다: $sourceDirectoryPath"
    }

    # COPYDIR과 동일하게 대상 폴더를 만들고 원본 폴더의 모든 내용을 복사합니다.
    New-Item -ItemType Directory -Path $destinationDirectoryPath -Force | Out-Null

    Get-ChildItem -LiteralPath $sourceDirectoryPath -Force | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $destinationDirectoryPath -Recurse -Force
    }

    Write-Host "리소스 복사 완료: $sourceDirectoryPath -> $destinationDirectoryPath"
}
