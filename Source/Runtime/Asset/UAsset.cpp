#include "UAsset.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UAsset, UObject)

void UAsset::LoadInternal(UAssetDesc& Desc)
{
	ID = Desc.ID;
	AssetPath = Desc.AssetPath;
	AssetSize = Desc.AssetSize;
}
