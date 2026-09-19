#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/FUObjectArray.h"
#include "Runtime/Asset/UAsset.h"
#include "Runtime/Rendering/FRenderPipeline.h"
#include "Runtime/Rendering/FTexture.h"
#include "Runtime/Rendering/FFont.h"
#include "Runtime/Material/FTextureSamplerDesc.h"

struct UFontDesc : UAssetDesc
{
	FName TextureFilePath; // dds, png file path
	FFont* Font = nullptr;
};

class UFont : public UAsset
{

	GENERATED_BODY()
	DECLARE_UCLASS(UFont, UAsset)

private:

	FFont* Font = nullptr;

public:

	void Load(UFontDesc& Desc);

};
