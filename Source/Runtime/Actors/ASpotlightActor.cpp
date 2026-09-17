#include "ASpotlightActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(ASpotlightActor, AActor)
UCLASS_META(ASpotlightActor, DisplayName, "Spotlight Actor")

ASpotlightActor::ASpotlightActor()
{
	CreateRootComponent(USpotLightComponent::StaticClass());
}

USpotLightComponent* ASpotlightActor::GetSpotlightComponent() const
{
	return RootComponent ? RootComponent->Cast<USpotLightComponent>() : nullptr;
}
