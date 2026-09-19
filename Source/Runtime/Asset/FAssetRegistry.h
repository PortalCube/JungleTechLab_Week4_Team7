#pragma once

#include "Runtime/Core/FName.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Rendering/FRenderPipeline.h"
#include "Runtime/Asset/UAsset.h"

#include "Runtime/Asset/UStaticMesh.h"
#include "Runtime/Asset/UMaterial.h"
#include "Runtime/Asset/UPipeline.h"
#include "Runtime/Asset/UFont.h"

// FObjManager 역할의 클래스
class FAssetRegistry
{
private:

	TMap<FName, UAsset*> AssetMap;

public:

	static FAssetRegistry& GetInstance();

	void Register(const FName& Name, UAsset* Pipeline);
	void Clear();
	
	template <typename T>
	T Get(const FName& Name);
	
};

template<typename T>
inline T FAssetRegistry::Get(const FName& Name)
{
	auto It = AssetMap.find(Name);

	if (It == AssetMap.end())
	{
		return nullptr;
	}

	UAsset* Asset = It->second;

	if (!Asset->IsA<T>())
	{
		return nullptr;
	}
	
	return Asset;
}
