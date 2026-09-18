#pragma once

#include "Runtime/Engine/FCamera.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Rendering/FRenderQueue.h"
#include "Runtime/Engine/ShowFlags.h"
#include "USceneComponent.h"

class UPrimitiveComponent : public USceneComponent {
  GENERATED_BODY()
  DECLARE_UCLASS(UPrimitiveComponent, USceneComponent)

public:
    void Initialize() override;
    void Register(UScene& InScene) override;
    void Unregister() override;

    virtual FMatrix GetRenderMatrix(const FCamera& Camera) const { return GetGlobalTransform().ToMatrix(); }

    virtual const FRenderData& GetRenderData(const FCamera& Camera){ return RenderData; }
    const FRenderData& GetPureRenderData() const { return RenderData; }

    virtual FAxisAlignedBoundingBox CalcLocalBounds() { return {}; }

    virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_Primitives; }

protected:
    UPrimitiveComponent() = default;

    FRenderData RenderData
    {
       .MeshId = FName("None"),
       .MaterialId = FName("None"),
       .TextureId = FName("None"),
       .type = ERenderType::None,
       .bSelected = false,
    };

    FVector Color{1.0f, 1.0f, 1.0f};
    float ColorAmount = 0.0f;
};
