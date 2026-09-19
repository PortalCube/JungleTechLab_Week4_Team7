#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/FUObjectArray.h"
#include "Runtime/Asset/UAsset.h"
#include "Runtime/Rendering/FRenderPipeline.h"
#include "Runtime/Rendering/FTexture.h"
#include "Runtime/Material/FTextureSamplerDesc.h"
#include "Runtime/Material/FBlendDesc.h"
#include "Runtime/Material/FRasterizerDesc.h"
#include "Runtime/Material/FDepthStencilDesc.h"

struct UPipelineDesc : UAssetDesc
{
	FName Pipeline;
	FString VertexShaderFilePath;
	FString PixelShaderFilePath;

	FBlendDesc Blend;
	FRasterizerDesc Rasterizer;
	FDepthStencilDesc DepthStencil;
};

class UPipeline : public UAsset
{

	GENERATED_BODY()
	DECLARE_UCLASS(UPipeline, UAsset)

private:

	FRenderPipeline* Pipeline = nullptr;

public:

	void Load(UPipelineDesc& Desc);

};