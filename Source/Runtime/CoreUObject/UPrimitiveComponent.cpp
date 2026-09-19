#include "UPrimitiveComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/CoreUObject/UClass.h"

IMPLEMENT_UCLASS(UPrimitiveComponent, USceneComponent)

void UPrimitiveComponent::Initialize()
{
    Super::Initialize();
    RenderData.Type = ERenderType::Primitive;

    FAssetRegistry& Registry = FAssetRegistry::GetInstance();
    FMaterialInstance DefaultMaterial
    {
        Registry.Get<UMaterial>("Material/Default_Material.json")
    };

    RenderData.Materials.push_back(DefaultMaterial);
}

void UPrimitiveComponent::Register(UScene& InScene)
{
    if (RenderData.Type == ERenderType::None)
    {
        RenderData.Type = (RenderData.Materials.size() > 0 && RenderData.Materials[0].Texture)
            ? ERenderType::Texture
            : ERenderType::Primitive;
    }

    Super::Register(InScene);
    InScene.AddRenderComponent(this);
}

void UPrimitiveComponent::Unregister()
{
    if (Scene)
    {
        Scene->RemoveRenderComponent(this);
    }
    Super::Unregister();
}