#include "ACubeActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"

IMPLEMENT_UCLASS(ACubeActor, AActor)
UCLASS_META(ACubeActor, DisplayName, "Cube Actor")

ACubeActor::ACubeActor()
{
	// 기본 큐브 컴포넌트 장착
	CreateRootComponent(UStaticMeshComponent::StaticClass());
}