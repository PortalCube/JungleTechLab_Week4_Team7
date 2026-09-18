#include "FMaterialInstance.h"

FMaterialInstance::FMaterialInstance(UMaterial* InMaterial)
	: Material{ InMaterial }
{
}

void FMaterialInstance::SetTexture(UTexture* InTexture)
{
	Texture = InTexture;
}

void FMaterialInstance::SetSamplerDesc(FTextureSamplerDesc InSamplerDesc)
{
	SamplerDesc = InSamplerDesc;
}
