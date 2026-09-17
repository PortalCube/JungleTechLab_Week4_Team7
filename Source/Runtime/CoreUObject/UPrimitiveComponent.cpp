#include "UPrimitiveComponent.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "UClass.h"
#include "Runtime/Engine/UScene.h"

IMPLEMENT_UCLASS(UPrimitiveComponent, USceneComponent)

void UPrimitiveComponent::Register(UScene& InScene)
{
	if (!PrimitiveMesh || !PrimitiveMaterial) { return; }

	Super::Register(InScene);
	InScene.AddRenderComponent(this);
}

void UPrimitiveComponent::Unregister()
{
	if (Scene)
	{
		Scene->RemoveRenderComponent(this);
	}

	Super::Unregister();
}
void UPrimitiveComponent::Render(FRenderer &renderer, const FCamera &Camera,
                                 const bool &bHighlighted) {

  if (!GetMesh() || !GetMaterial()) {
    return;
  }

  const FMatrix VP = Camera.CreateViewProjectionMatrix();
  const FMatrix World = GetRenderMatrix(Camera);

  FObjectConstants Constants;
  Constants.MVP = World * VP;
  Constants.World = World;

  // 컴포넌트 색상 반영
  Constants.ColorOverride = GetColor();
  Constants.ColorOverrideAmount = GetColorAmount();

  if (bHighlighted) {
    // 하이라이트 색상 보정
    if (Constants.ColorOverrideAmount > 0.0f) {
      Constants.ColorOverride =
          Constants.ColorOverride * 0.7f + FVector{0.3f, 0.3f, 0.3f};
    } else {
      Constants.ColorOverride = FVector{1.0f, 1.0f, 1.0f};
      Constants.ColorOverrideAmount = 0.5f;
    }
  }

  renderer.Draw(*GetMesh(), *GetMaterial(), Constants);
}

void UPrimitiveComponent::SetRelativeTransform(
    const FTransform &RelativeTransform) {
  Super::SetRelativeTransform(RelativeTransform);
}

bool UPrimitiveComponent::SetTextureByName(const FString &InTextureName) {
  auto CurrentMat = GetMaterial();
  if (!CurrentMat) {
    return false;
  }

  auto &ResLib = FRenderResourceLibrary::Get();

  // 독립 머티리얼 인스턴스 생성 및 텍스처 교체
  auto NewMaterial = TSharedPtr<FMaterial>(new FMaterial());
  NewMaterial->SetPipeLine(CurrentMat->GetPipeline());

  if (!NewMaterial->SetTextureByName(InTextureName)) {
    return false;
  }

  SetMaterial(NewMaterial);
  return true;
}
