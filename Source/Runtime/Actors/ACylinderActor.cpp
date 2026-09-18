#include "ACylinderActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(ACylinderActor, AActor)
UCLASS_META(ACylinderActor, DisplayName, "Cylinder Actor")

ACylinderActor::ACylinderActor()
{
	// 기본 실린더 컴포넌트 장착
	CreateRootComponent(UCylinderComp::StaticClass());
}

UCylinderComp* ACylinderActor::GetCylinderComponent() const
{
	return RootComponent ? RootComponent->Cast<UCylinderComp>() : nullptr;
}
