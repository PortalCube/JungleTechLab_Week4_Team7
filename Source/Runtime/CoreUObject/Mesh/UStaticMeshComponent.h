
#pragma once

#include "Runtime/CoreUObject/Mesh/UMeshComponent.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Material/FMaterialInstance.h"

class UStaticMeshComponent : public UMeshComponent {
    GENERATED_BODY()
    DECLARE_UCLASS(UStaticMeshComponent, UMeshComponent)

public:
    virtual void SetMesh(UStaticMesh* Mesh) override;

    virtual const UStaticMesh* GetMesh() override { return RenderData.Mesh; }
    virtual const UMaterial* GetMaterial(int Index = 0) const override;
    virtual const FMaterialInstance* GetMaterialInstance(int Index = 0) const override;
    virtual const TArray<FMaterialInstance>* GetAllMaterialInstance() const override { return &RenderData.Materials; }

    virtual const FRenderData& GetRenderData(const FCamera& Camera) const override;

    void ClearMaterial();

    int32 GetMaterialSlotLength() const { return static_cast<int32>(RenderData.Materials.size()); }

    virtual EEngineShowFlags GetShowFlag() const;

protected:
    UStaticMeshComponent() = default;
};
