
#pragma once

#include "Runtime/CoreUObject/Mesh/UMeshComponent.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Core/TArray.h"

class UStaticMeshComponent : public UMeshComponent {
    GENERATED_BODY()
    DECLARE_UCLASS(UStaticMeshComponent, UMeshComponent)

public:
    void Initialize() override;
    void Register(UScene& InScene) override;
    void Unregister() override;

    virtual const UMesh* GetMesh() override { return Mesh; }
    virtual const UMaterial* GetMaterial(int Index = 0) const override { return MaterialInstances[Index].GetMaterial(); }
    virtual const FMaterialInstance* GetMaterialInstance(int Index = 0) const override { return &MaterialInstances[Index]; }
    virtual const TArray<FMaterialInstance>* GetAllMaterialInstance() const override { return &MaterialInstances; }

    void SetMesh(UMesh* InMesh);
    void SetMaterial(UMaterial* InMaterial, int Index = 0);
    void ClearMaterial();

    int GetMaterialSlotLength() const;

    virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_Mesh; }

protected:
    UStaticMeshComponent() = default;

    UMesh* Mesh = nullptr;
    TArray<FMaterialInstance> MaterialInstances;
};
