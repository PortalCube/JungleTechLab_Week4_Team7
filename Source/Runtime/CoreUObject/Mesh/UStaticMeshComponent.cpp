#include "UStaticMeshComponent.h"
IMPLEMENT_UCLASS(UStaticMeshComponent, UMeshComponent)
#include "../../Asset/FAssetRegistry.h"

const FRenderData& UStaticMeshComponent::GetRenderData(const FCamera& Camera) const
{
	RenderData.ModelMatrix = GetRenderMatrix(Camera);
	return RenderData;
}

void UStaticMeshComponent::SetMesh(UStaticMesh* Mesh)
{
	UPrimitiveComponent::SetMesh(Mesh);

	const TArray<FMeshSection>& Sections = Mesh->Get()->GetSections();
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	RenderData.Materials.clear();
	RenderData.Materials.reserve(Sections.size());
	for (size_t i = 0; i < Sections.size(); i++)
	{
		UMaterial* Mat = Registry.Get<UMaterial>(FName(Sections[i].SectionName));
		SetMaterial(Mat, static_cast<int32>(i));
	}
}

const UMaterial* UStaticMeshComponent::GetMaterial(int Index) const
{
	const FMaterialInstance* Instance = GetMaterialInstance(Index);
	return Instance ? Instance->Material : nullptr;
}

const FMaterialInstance* UStaticMeshComponent::GetMaterialInstance(int Index) const
{
	if (Index < 0 || Index >= GetMaterialSlotLength()) { return nullptr; }
	return &RenderData.Materials[static_cast<size_t>(Index)];
}

void UStaticMeshComponent::ClearMaterial()
{
	RenderData.Materials.clear();
}

EEngineShowFlags UStaticMeshComponent::GetShowFlag() const
{
	return EEngineShowFlags::SF_Primitives;
}

