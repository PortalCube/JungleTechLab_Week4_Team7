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

	TMap<FName, UPipeline*> PipelineMap;
	TMap<FName, UStaticMesh*> StaticMeshMap;
	TMap<FName, UMaterial*> MaterialMap;
	TMap<FName, UFont*> FontMap;

public:

	// TODO: 4개의 맵과 4개의 Register, Get, Clear 함수...
	// 리팩토링이 필요할 것

	static FAssetRegistry& Get();

	// StaticMesh
	void RegisterPipeline(FName& Name, UPipeline* Pipeline);
	UPipeline* GetPipeline(FName& Name);
	void ClearPipeline();

	// StaticMesh
	void RegisterStaticMesh(FName& Name, UStaticMesh* StaticMesh);
	UStaticMesh* GetStaticMesh(FName& Name);
	void ClearStaticMesh();
	
	// Material
	void RegisterMaterial(FName& Name, UMaterial* Material);
	UMaterial* GetMaterial(FName& Name);
	void ClearMaterial();

	// Font
	void RegisterFont(FName& Name, UFont* Font);
	UFont* GetFont(FName& Name);
	void ClearFont();

	void ClearAll();
	
};