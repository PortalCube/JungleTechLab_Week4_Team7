#pragma once

#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Rendering/FMaterial.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Engine/ShowFlags.h"
#include "USceneComponent.h"


class FRenderer;

class UPrimitiveComponent : public USceneComponent {
  GENERATED_BODY()
  DECLARE_UCLASS(UPrimitiveComponent, USceneComponent)

public:
	void Register(UScene& InScene) override;
	void Unregister() override;

	[[nodiscard]] TSharedPtr<FMesh> GetMesh() const { return PrimitiveMesh; }
	[[nodiscard]] TSharedPtr<FMaterial> GetMaterial() const { return PrimitiveMaterial; }
	[[nodiscard]] FMatrix GetModelMatrix() const { return GetGlobalTransform().ToMatrix(); }
	virtual FMatrix GetRenderMatrix(const FCamera& Camera) { return GetGlobalTransform().ToMatrix(); }
	virtual void SetRelativeTransform(const FTransform& RelativeTransform) override;

  // 컴포넌트 렌더링
  virtual void Render(FRenderer &renderer, const FCamera &Camera,
                      const bool &bHighlighted);

  // 메쉬 및 재질 설정
  void SetMesh(TSharedPtr<FMesh> Mesh) { PrimitiveMesh = std::move(Mesh); }
  void SetMaterial(TSharedPtr<FMaterial> Material) {
    PrimitiveMaterial = std::move(Material);
  }

  // 텍스처 이름으로 머티리얼 텍스처 교체
  bool SetTextureByName(const FString &InTextureName);

  // 색상 설정 및 조회
  const FVector &GetColor() const { return Color; }
  void SetColor(const FVector &InColor) {
    Color = InColor;
    ColorAmount = 1.0f;
  }
  float GetColorAmount() const { return ColorAmount; }
  void SetColorAmount(float InAmount) { ColorAmount = InAmount; }


  virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_Primitives; }

protected:
  UPrimitiveComponent() = default;

  TSharedPtr<FMesh> PrimitiveMesh;
  TSharedPtr<FMaterial> PrimitiveMaterial;
  TSharedPtr<FAxisAlignedBoundingBox> BoundingBox;

  FVector Color{1.0f, 1.0f, 1.0f};
  float ColorAmount = 0.0f;
};
