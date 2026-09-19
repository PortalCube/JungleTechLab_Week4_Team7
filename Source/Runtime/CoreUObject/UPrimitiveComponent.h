#pragma once

#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/FRenderData.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "USceneComponent.h"

class UPrimitiveComponent : public USceneComponent {
  GENERATED_BODY()
  DECLARE_UCLASS(UPrimitiveComponent, USceneComponent)

public:
    void Initialize() override;
    void Register(UScene& InScene) override;
    void Unregister() override;


    virtual const FRenderData& GetRenderData(const FCamera& Camera) const { return RenderData; }
    virtual FMatrix GetRenderMatrix(const FCamera& Camera) const { return GetGlobalTransform().ToMatrix(); }

    virtual FAxisAlignedBoundingBox CalcLocalBounds() { return {}; }

    virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_Primitives; }

protected:
    UPrimitiveComponent() = default;

    mutable FRenderData RenderData
    {
       .Mesh = nullptr,
       .Materials = {},
       .ModelMatrix = FMatrix::Identity,
       .Type = ERenderType::None,
    };

    FVector Color{1.0f, 1.0f, 1.0f};
    float ColorAmount = 0.0f;
};
