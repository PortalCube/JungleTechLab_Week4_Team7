#include "ACylinderActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"

IMPLEMENT_UCLASS(ACylinderActor, AActor)
UCLASS_META(ACylinderActor, DisplayName, "Cylinder Actor")

ACylinderActor::ACylinderActor()
{
	// 기본 실린더 컴포넌트 장착
	CreateRootComponent(UStaticMeshComponent::StaticClass());
}
