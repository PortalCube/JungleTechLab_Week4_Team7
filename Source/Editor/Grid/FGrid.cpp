#include "FGrid.h"

#include "Editor/Core/FEditor.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector4.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include <cmath>
#include <numbers>


#include "Runtime/Engine/FRayCastingManager.h"

void FGrid::Initialize() {
  auto &RenderResources = FRenderResourceLibrary::Get();
  GridMesh = RenderResources.GetMesh(EMeshID::Grid);
  GridMaterial = RenderResources.GetMaterial(EMaterialID::Grid);
  LineMesh = RenderResources.GetMesh(EMeshID::Arrow);
  LineMaterial = RenderResources.GetMaterial(EMaterialID::Simple);
}

void FGrid::Draw(FRenderer &Renderer, const FCamera &Camera) {
  if (!GridMesh || !GridMaterial)
    return;

  // 카메라 XY 따라감, Z=0 (바닥)
  const float ScaleFactor = (CellSize > 1.0f) ? CellSize : 1.0f;
  const FMatrix World =
      FMatrix::MakeScale(FVector{ScaleFactor, ScaleFactor, 1.0f}) *
      FMatrix::MakeTranslation(
          FVector{Camera.Position.X, Camera.Position.Y, 0.0f});

  const FMatrix VP = Camera.CreateViewProjectionMatrix();

  FGridConstants C;
  C.World = World;
  C.MVP = World * VP; // 스왑은 UpdateGridConstants 가 함
  C.CellSize = CellSize;

  Renderer.Draw(*GridMesh, *GridMaterial, C, 0, false);

  if (!LineMesh || !LineMaterial)
    return;

  const FMatrix LineMatrix =
      FMatrix::MakeTranslation(FVector{-0.5f, 0.0f, 0.0f}) *
      FMatrix::MakeScale(FVector{100.0f * ScaleFactor, 0.5f * ScaleFactor,
                                 0.5f * ScaleFactor});
  Renderer.Draw<FObjectConstants>(
      *LineMesh, *LineMaterial,
      {LineMatrix *
           FMatrix::MakeTranslation(FVector{Camera.Position.X, 0.0f, 0.0f}) *
           VP,
       FVector{1.0f, 0.0f, 0.0f}, 1.0f});
  Renderer.Draw<FObjectConstants>(
      *LineMesh, *LineMaterial,
      {LineMatrix * FMatrix::MakeRotationZ(std::numbers::pi_v<float> * 0.5f) *
           FMatrix::MakeTranslation(FVector{0.0f, Camera.Position.Y, 0.0f}) *
           VP,
       FVector{0.0f, 1.0f, 0.0f}, 1.0f});
  Renderer.Draw<FObjectConstants>(
      *LineMesh, *LineMaterial,
      {LineMatrix * FMatrix::MakeRotationY(std::numbers::pi_v<float> * 0.5f) *
           FMatrix::MakeTranslation(FVector{0.0f, 0.0f, Camera.Position.Z}) *
           VP,
       FVector{0.0f, 0.0f, 1.0f}, 1.0f});
}

void FGrid::DrawLine(FRenderer &Renderer, const FCamera &Camera) {
  auto &LineBatcher = Renderer.GetLineBatcher();

  const int32 HalfLineCount = static_cast<int32>(50.0f / CellSize);
  const float Extent = HalfLineCount * CellSize;

  const float SnapX = std::floor(Camera.Position.X / CellSize) * CellSize;
  const float SnapY = std::floor(Camera.Position.Y / CellSize) * CellSize;

  const FVector4 MinorGridColor{0.2f, 0.2f, 0.2f, 1.0f};
  const FVector4 MajorGridColor{0.55f, 0.55f, 0.55f, 1.0f};
  const FVector4 AxisColorX{0.8f, 0.2f, 0.2f, 1.0f};
  const FVector4 AxisColorY{0.2f, 0.8f, 0.2f, 1.0f};

  const bool bEnableMajorGrid = (CellSize <= 0.2f);
  const float LineOffset = CellSize * 0.02f;

  // 가로선 렌더링
  for (int32 i = -HalfLineCount; i <= HalfLineCount; ++i) {
    float Y = SnapY + i * CellSize;
    if (std::abs(Y) < 0.001f) {
      LineBatcher.DrawLine(FVector{SnapX - Extent, Y, 0.0f},
                           FVector{SnapX + Extent, Y, 0.0f}, AxisColorX);
    } else {
      const int64 GridIndex = static_cast<int64>(std::round(Y / CellSize));
      const bool bIsMajor = bEnableMajorGrid && (std::abs(GridIndex) % 10 == 0);
      const FVector4 Color = bIsMajor ? MajorGridColor : MinorGridColor;

      LineBatcher.DrawLine(FVector{SnapX - Extent, Y, 0.0f},
                           FVector{SnapX + Extent, Y, 0.0f}, Color);
      if (bIsMajor) {
        LineBatcher.DrawLine(FVector{SnapX - Extent, Y - LineOffset, 0.0f},
                             FVector{SnapX + Extent, Y - LineOffset, 0.0f}, Color);
        LineBatcher.DrawLine(FVector{SnapX - Extent, Y + LineOffset, 0.0f},
                             FVector{SnapX + Extent, Y + LineOffset, 0.0f}, Color);
      }
    }
  }

  // 세로선 렌더링
  for (int32 i = -HalfLineCount; i <= HalfLineCount; ++i) {
    float X = SnapX + i * CellSize;
    if (std::abs(X) < 0.001f) {
      LineBatcher.DrawLine(FVector{X, SnapY - Extent, 0.0f},
                           FVector{X, SnapY + Extent, 0.0f}, AxisColorY);
    } else {
      const int64 GridIndex = static_cast<int64>(std::round(X / CellSize));
      const bool bIsMajor = bEnableMajorGrid && (std::abs(GridIndex) % 10 == 0);
      const FVector4 Color = bIsMajor ? MajorGridColor : MinorGridColor;

      LineBatcher.DrawLine(FVector{X, SnapY - Extent, 0.0f},
                           FVector{X, SnapY + Extent, 0.0f}, Color);
      if (bIsMajor) {
        LineBatcher.DrawLine(FVector{X - LineOffset, SnapY - Extent, 0.0f},
                             FVector{X - LineOffset, SnapY + Extent, 0.0f}, Color);
        LineBatcher.DrawLine(FVector{X + LineOffset, SnapY - Extent, 0.0f},
                             FVector{X + LineOffset, SnapY + Extent, 0.0f}, Color);
      }
    }
  }

  // 수직 축선 렌더링
  LineBatcher.DrawLine(FVector{0.0f, 0.0f, -Extent},
                       FVector{0.0f, 0.0f, Extent},
                       FVector4{0.2f, 0.4f, 0.9f, 1.0f});
}
