#include "AInstancingActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UInstancePrimitiveComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AInstancingActor, AActor)
UCLASS_META(AInstancingActor, DisplayName, "Instancing Actor")

AInstancingActor::AInstancingActor()
{
	CreateRootComponent(UInstancePrimitiveComponent::StaticClass());

	if (auto* PrimComp = GetRootComponent()->Cast<UInstancePrimitiveComponent>())
	{
		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		PrimComp->SetMesh(Registry.Get<UStaticMesh>("#MasterYi"));
		PrimComp->SetMaterial(Registry.Get<UMaterial>("Material/Instance_Textured.json"));
	}

	FTransform DefaultTransform;
	DefaultTransform.Scale3D = FVector(5.0f, 5.0f, 5.0f);
	SetTransform(DefaultTransform);

}
