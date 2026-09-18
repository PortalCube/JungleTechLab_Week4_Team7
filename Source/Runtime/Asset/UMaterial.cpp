#include "UMaterial.h"

void UMaterial::Load(UMaterialDesc& Desc)
{
	LoadInternal(Desc);
	Desc.Pipeline = Desc.Pipeline;
	Desc.Texture = Desc.Texture;
	Desc.SamplerDesc = Desc.SamplerDesc;
}
