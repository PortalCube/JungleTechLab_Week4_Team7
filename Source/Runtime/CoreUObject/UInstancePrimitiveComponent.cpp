#include "UInstancePrimitiveComponent.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Engine/UScene.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UInstancePrimitiveComponent, UPrimitiveComponent)

void UInstancePrimitiveComponent::Register(UScene& Scene)
{
	FRenderResourceLibrary* Resources = Scene.GetRenderResourceLibrary();
	if (!PrimitiveMesh)
	{
		SetMesh(Resources ? Resources->GetMesh(EMeshID::Cube) : nullptr);
	}
	
	if (!PrimitiveMaterial)
	{
		SetMaterial(Resources ? Resources->GetMaterial(EMaterialID::Instance_Simple) : nullptr);
	}

	Super::Register(Scene);
}

void UInstancePrimitiveComponent::Render(FRenderer& renderer, const FCamera& Camera, const bool& bHighlighted)
{
	// 매 프레임 이전 인스턴스 누적 방지
	Instances.clear();

	FMatrix WorldMatrix = GetGlobalTransform().ToMatrix();
	const auto& Positions = GetMesh()->GetPositions();
	TArray<FVector> WorldPositions;
	
	WorldPositions.reserve(Positions.size());
	for (const FVector& LocalPos : Positions)
	{
		// 정점 좌표 변환 (W = 1.0f 기준)
		FVector WorldPos = WorldMatrix.TransformPointRow(LocalPos);
		FMatrix PosMatrix = FMatrix::MakeTranslation(WorldPos);
		FInstanceData Data
		{
			.World = PosMatrix,
			.Color = FVector4(GetColor(), 1.0f),
		};

		Instances.push_back(Data);
	}


	renderer.AddTextInstanceArray(Instances, GetMesh()->MeshId, GetMaterial()->MaterialId);
}
