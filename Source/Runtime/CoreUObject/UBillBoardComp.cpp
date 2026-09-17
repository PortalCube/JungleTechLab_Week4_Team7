#include "UBillBoardComp.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Rendering/FRenderer.h"
#include "UClass.h"
#include <algorithm>
#include <cctype>

IMPLEMENT_UCLASS(UBillBoardComp, UPrimitiveComponent)
UCLASS_META(UBillBoardComp, DisplayName, "BillBoard")
UCLASS_META(UBillBoardComp, MeshName, "BillBoard")

void UBillBoardComp::Register(UScene& InScene) {
  FRenderResourceLibrary* Resources = InScene.GetRenderResourceLibrary();
  if (!GetMesh()) {
    SetMesh(Resources ? Resources->GetMesh(EMeshID::Rect) : nullptr);
  }
  if (!GetMaterial()) {
    SetMaterial(Resources ? Resources->GetMaterial(EMaterialID::Billboard) : nullptr);
  }
  Super::Register(InScene);
}

void UBillBoardComp::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);

    Archive.SetVector2("UVScale", UVScale);
    Archive.SetVector2("UVOffset", UVOffset);
}

void UBillBoardComp::Deserialize(const FArchive& Archive)
{
    Super::Deserialize(Archive);

    UVScale = Archive.GetVector2("UVScale");
    UVOffset = Archive.GetVector2("UVOffset");
}

void UBillBoardComp::Render(FRenderer &renderer, const FCamera &Camera, const bool &bHighlighted) {
  if (!GetMesh() || !GetMaterial()) {
    return;
  }

  FTransform Transform = GetGlobalTransform();

  FMatrix CameraRotation = Camera.GetRotationMatrix();
  FVector ViewForward = CameraRotation.TransformPointRow(FVector{ 1.0f, 0.0f, 0.0f }, 0.0f); // X+
  FVector ViewRight = CameraRotation.TransformPointRow(FVector{ 0.0f, 1.0f, 0.0f }, 0.0f); // Y+
  FVector ViewUp = CameraRotation.TransformPointRow(FVector{ 0.0f, 0.0f, 1.0f }, 0.0f); // Z+

  FVector Up = ViewUp * Transform.Scale3D.Z;
  FVector Right = ViewRight * Transform.Scale3D.Y;

  FMatrix ModelMatrix
  {
      FVector4{ ViewForward, 0.0f },
      FVector4{ Right, 0.0f },
      FVector4{ Up, 0.0f },
      FVector4{ Transform.Location, 1.0f },
  };

  FMatrix VP = Camera.CreateViewProjectionMatrix();

  FObjectConstants Constants;
  Constants.MVP = ModelMatrix * VP;
  Constants.World = ModelMatrix;


  // UV 반영
  Constants.UVScale = UVScale;
  Constants.UVOffset = UVOffset;

  // 컴포넌트 색상 반영
  Constants.ColorOverride = GetColor();
  Constants.ColorOverrideAmount = GetColorAmount();

  if (bHighlighted) {
      // 하이라이트 색상 보정
      if (Constants.ColorOverrideAmount > 0.0f) {
          Constants.ColorOverride =
              Constants.ColorOverride * 0.7f + FVector{ 0.3f, 0.3f, 0.3f };
      }
      else {
          Constants.ColorOverride = FVector{ 1.0f, 1.0f, 1.0f };
          Constants.ColorOverrideAmount = 0.5f;
      }
  }

  renderer.Draw(*GetMesh(), *GetMaterial(), Constants);
}

void UBillBoardComp::SetTexture(
    FString texture) // 원본 머터리얼을 건드리지 않고 instance로 생성해서 사용
{
  auto &lib = FRenderResourceLibrary::Get();

  // 소문자 변환
  FString LowerName = texture;
  std::transform(LowerName.begin(), LowerName.end(), LowerName.begin(),
                 ::tolower);

  auto NewTex = lib.GetTexture(LowerName);
  if (!NewTex) {
    UE_LOG("There is no such texture");
    return;
  }

  // 머티리얼 인스턴스 생성
  auto materialinstance = TSharedPtr<FMaterial>(new FMaterial());

  if (GetMaterial()) {
    materialinstance->SetPipeLine(GetMaterial()->GetPipeline());
  } else {
    materialinstance->SetPipeLine(lib.GetPipeline(EPipelineID::Textured));
  }

  materialinstance->SetTexture(NewTex);
  SetMaterial(materialinstance);
}
