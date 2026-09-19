#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/FUObjectArray.h"
#include "Runtime/Asset/UAsset.h"
#include "Runtime/Rendering/FRenderPipeline.h"
#include "Runtime/Rendering/FTexture.h"
#include "Runtime/Material/FTextureSamplerDesc.h"

struct UMaterialDesc : UAssetDesc
{
	FName PipelineFilePath;
	FName TextureFilePath;
	FTextureSamplerDesc TextureSamplerDesc;
};

class UMaterial : public UAsset
{

	GENERATED_BODY()
	DECLARE_UCLASS(UMaterial, UAsset)

private:

	FRenderPipeline* Pipeline = nullptr;
	FTexture* Texture = nullptr;
	FTextureSamplerDesc SamplerDesc{};

public:

	void Load(UMaterialDesc& Desc);

};