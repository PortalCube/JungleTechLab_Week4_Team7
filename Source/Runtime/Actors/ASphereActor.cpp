#include "ASphereActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(ASphereActor, AActor)
UCLASS_META(ASphereActor, DisplayName, "Sphere Actor")

ASphereActor::ASphereActor()
{
	// 기본 구체 컴포넌트 장착
	CreateRootComponent(USphereComp::StaticClass());
}

USphereComp* ASphereActor::GetSphereComponent() const
{
	return RootComponent ? RootComponent->Cast<USphereComp>() : nullptr;
}
