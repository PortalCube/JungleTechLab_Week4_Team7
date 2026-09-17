#pragma once
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Math/FQuaternion.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "UPrimitiveComponent.h"

class UScene;
class FArchive;

class UBillBoardComp : public UPrimitiveComponent {
  DECLARE_UCLASS(UBillBoardComp, UPrimitiveComponent)
  GENERATED_BODY()

protected:
  explicit UBillBoardComp() = default;

  virtual void Serialize(FArchive& Archive) const;
  virtual void Deserialize(const FArchive& Archive);

  // 텍스처 좌표 속성
  FVector2 UVScale{1.0f, 1.0f};
  FVector2 UVOffset{0.0f, 0.0f};

public:
	virtual void Register(UScene& InScene) override;

  // 빌보드 렌더링
  virtual void Render(FRenderer &renderer, const FCamera &Camera,
              const bool &bHighlighted) override;

  virtual void SetTexture(FString texture); // 원본 머터리얼을 건드리지 않고
                                            // instance로 생성해서 사용



  virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_BillboardText; }
private:
  // 시선 회전 보간용 쿼터니언
  FQuaternion CurrentRotation = FQuaternion::Identity();
};

