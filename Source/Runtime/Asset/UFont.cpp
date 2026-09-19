#include "UFont.h"

void UFont::Load(UFontDesc& Desc)
{
	LoadInternal(Desc);
	Texture = Desc.Texture;
	Font = Desc.Font;
}
