#include "UStaticMeshComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Material/FMaterialInstance.h"
#include "Runtime/Asset/UStaticMesh.h"

#include <stdexcept>

IMPLEMENT_UCLASS(UStaticMeshComponent, UMeshComponent)

const FRenderData& UStaticMeshComponent::GetRenderData(const FCamera& Camera) const
{
	return RenderData;
}

void UStaticMeshComponent::SetMesh(UStaticMesh* InMesh)
{
	RenderData.Mesh = InMesh;
}

void UStaticMeshComponent::SetMaterial(UMaterial* InMaterial, int Index)
{
	if (GetMaterialSlotLength() <= Index)
	{
		throw EngineUtil::CreateError("Index가 범위를 초과했습니다. 슬롯 갯수: {}, Index: {}", GetMaterialSlotLength(), Index);
	}

	RenderData.Materials[Index] = FMaterialInstance{ InMaterial };
}

void UStaticMeshComponent::ClearMaterial()
{
	RenderData.Materials.clear();
}
