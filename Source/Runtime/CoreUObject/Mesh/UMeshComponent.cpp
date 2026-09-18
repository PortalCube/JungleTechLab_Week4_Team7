#include "UMeshComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UMeshComponent, UPrimitiveComponent)

void UMeshComponent::Initialize()
{
	Super::Initialize();
}

void UMeshComponent::Register(UScene& InScene)
{
	Super::Register(InScene);
}

void UMeshComponent::Unregister()
{
	Super::Unregister();
}