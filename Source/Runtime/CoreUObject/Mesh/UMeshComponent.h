#pragma once


#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Rendering/FRenderQueue.h"
#include "Runtime/Engine/ShowFlags.h"

class UMesh;
class UMaterial;
class FMaterialInstance;

class UMeshComponent : public UPrimitiveComponent {
    GENERATED_BODY()
    DECLARE_UCLASS(UMeshComponent, UPrimitiveComponent)

public:
    void Initialize() override;
    void Register(UScene& InScene) override;
    void Unregister() override;

    virtual const UMesh* GetMesh() { return nullptr; }
    virtual const UMaterial* GetMaterial(int Index = 0) const { return nullptr; }
    virtual const FMaterialInstance* GetMaterialInstance(int Index = 0) const { return nullptr; }
    virtual const TArray<FMaterialInstance>* GetAllMaterialInstance() const { return nullptr; }

    virtual const FRenderData& GetRenderData(const FCamera& Camera) { return RenderData; }

    virtual EEngineShowFlags GetShowFlag() const override { return EEngineShowFlags::SF_Mesh; }

protected:
    UMeshComponent() = default;
};
