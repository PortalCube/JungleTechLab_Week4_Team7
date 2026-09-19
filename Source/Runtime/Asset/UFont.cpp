#include "UFont.h"

void UFont::Load(UFontDesc& Desc)
{
	LoadInternal(Desc);
	Font = Desc.Font;
}
