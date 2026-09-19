
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

    virtual const UStaticMesh* GetMesh() override { return RenderData.Mesh; }
    virtual const UMaterial* GetMaterial(int Index = 0) const override { return RenderData.Materials[Index].Material; }
    virtual const FMaterialInstance* GetMaterialInstance(int Index = 0) const override { return &RenderData.Materials[Index]; }
    virtual const TArray<FMaterialInstance>* GetAllMaterialInstance() const override { return &RenderData.Materials; }

    virtual const FRenderData& GetRenderData(const FCamera& Camera) const override;

    void SetMesh(UStaticMesh* InMesh);
    void SetMaterial(UMaterial* InMaterial, int Index = 0);
    void ClearMaterial();

    // TODO
    int GetMaterialSlotLength() const { return 0; }

    virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_Mesh; }

protected:
    UStaticMeshComponent() = default;
};
