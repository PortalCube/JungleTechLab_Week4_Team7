#include "UMaterial.h"

void UMaterial::Load(UMaterialDesc& Desc)
{
	LoadInternal(Desc);
	Pipeline = Desc.Pipeline;
	Texture = Desc.Texture;
	SamplerDesc = Desc.TextureSamplerDesc;
}
