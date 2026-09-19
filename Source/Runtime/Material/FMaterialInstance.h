#pragma once

#include "Runtime/Material/FTextureSamplerDesc.h"

class UMaterial;
class UTexture;
class UPipeline;

class FMaterialInstance
{
private:
	
	UMaterial* Material;

	UPipeline* Pipeline;
	UTexture* Texture;
	FTextureSamplerDesc SamplerDesc;

	// TODO: 재질에 대한 Parameter (Albedo, Diffuse, Specular) 받아오는 기능 추가.
	// 쉐이더마다 받아올 Parameter가 다르므로.. 굉장히 복잡한 기능이 될 수 있음

public:

	FMaterialInstance(UMaterial* InMaterial);

	UMaterial* GetMaterial() const { return Material; }
	UTexture* GetTexture() const { return Texture; }
	const FTextureSamplerDesc GetSamplerDesc() const { return SamplerDesc; }

	void SetTexture(UTexture* InTexture);
	void SetSamplerDesc(FTextureSamplerDesc InSamplerDesc);
};