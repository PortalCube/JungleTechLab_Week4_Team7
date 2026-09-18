#include "UMeshComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UStaticMeshComponent.h"
#include "Runtime/Material/FMaterialInstance.h"

#include <stdexcept>

IMPLEMENT_UCLASS(UStaticMeshComponent, UMeshComponent)

void UStaticMeshComponent::Initialize()
{
	Super::Initialize();
}

void UStaticMeshComponent::Register(UScene& InScene)
{
	Super::Register(InScene);
}

void UStaticMeshComponent::Unregister()
{
	Super::Unregister();
}

void UStaticMeshComponent::SetMesh(UMesh* InMesh)
{
	Mesh = InMesh;
}

void UStaticMeshComponent::SetMaterial(UMaterial* InMaterial, int Index)
{
	if (GetMaterialSlotLength() <= Index)
	{
		throw EngineUtil::CreateError("Index가 범위를 초과했습니다. 슬롯 갯수: {}, Index: {}", GetMaterialSlotLength(), Index);
	}

	MaterialInstances[Index] = FMaterialInstance{ InMaterial };
}
