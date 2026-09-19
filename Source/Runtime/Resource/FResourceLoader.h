#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/FString.h"

class FArchive;

class FResourceLoader
{

private:

	static constexpr int32 CurrentSchemaVersion = 1;
	static constexpr FStringView AssetDirectoryPath = "Assets";

	static void LoadPipelineAsset(FArchive& Archive);
	static void LoadMaterialAsset(FArchive& Archive);
	static void LoadStaticMeshAsset(FArchive& Archive);
	static void LoadFontAsset(FArchive& Archive);
	static void LoadTextureAsset(FArchive& Archive);

public:
	/// <summary>
	/// 엔진에서 기본으로 사용하는 메쉬를 생성하고 UStaticMesh 애셋으로 등록합니다.
	/// </summary>
	static void LoadDefaultStaticMeshAssets();

	/// <summary>
	/// AssetPath의 모든 애셋을 로드합니다. 
	/// </summary>
	static void LoadAssets();

};
