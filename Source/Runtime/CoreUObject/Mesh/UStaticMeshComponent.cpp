#include "UStaticMeshComponent.h"
IMPLEMENT_UCLASS(UStaticMeshComponent, UMeshComponent)

const FRenderData& UStaticMeshComponent::GetRenderData(const FCamera& Camera) const
{
	RenderData.ModelMatrix = GetRenderMatrix(Camera);
	return RenderData;
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

void UStaticMeshComponent::SetPipeline(UPipeline* Pipeline, int Index)
{
	if (Index < 0 || Index >= GetMaterialSlotLength()) { return; }
	FMaterialInstance& Instance = RenderData.Materials[static_cast<size_t>(Index)];

	Instance.Pipeline = Pipeline;
}

void UStaticMeshComponent::SetTexture(UTexture* Texture, int Index)
{
	if (Index < 0 || Index >= GetMaterialSlotLength()) { return; }
	FMaterialInstance& Instance = RenderData.Materials[static_cast<size_t>(Index)];

	Instance.Texture = Texture;
}

void UStaticMeshComponent::ClearMaterial()
{
	RenderData.Materials.clear();
}

EEngineShowFlags UStaticMeshComponent::GetShowFlag() const
{
	return EEngineShowFlags::SF_Primitives;
}

