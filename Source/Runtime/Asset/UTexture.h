#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/FUObjectArray.h"
#include "Runtime/Asset/UAsset.h"
#include "Runtime/Rendering/FTexture.h"

struct UTextureDesc : UAssetDesc
{
	FName RawTextureFilePath; // png, jpg, dds file path
	FTexture* Texture = nullptr;
};

class UTexture : public UAsset
{
	GENERATED_BODY()
	DECLARE_UCLASS(UTexture, UAsset)

private:
	FTexture* Texture = nullptr;

public:
	void Load(UTextureDesc& Desc);
};
