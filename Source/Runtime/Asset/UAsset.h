#pragma once

#include "Runtime/CoreUObject/UObject.h"

#include "Runtime/Core/FName.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"

struct UAssetDesc
{
	FName ID			= "";
	FString AssetPath	= "";
	uint64 AssetSize	= 0;
};

class UAsset : public UObject
{

	GENERATED_BODY()
	DECLARE_UCLASS(UAsset, UObject)

protected:

	FName ID			= "";
	FString AssetPath	= "";
	uint64 AssetSize	= 0;

	void LoadInternal(UAssetDesc& Desc);

};