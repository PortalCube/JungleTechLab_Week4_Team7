#include "AInstancingActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UInstancePrimitiveComponent.h"

IMPLEMENT_UCLASS(AInstancingActor, AActor)
UCLASS_META(AInstancingActor, DisplayName, "Instancing Actor")

AInstancingActor::AInstancingActor()
{
	CreateRootComponent(UInstancePrimitiveComponent::StaticClass());

}
