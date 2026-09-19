#include "FAssetRegistry.h"
#include "Runtime/Utility/EngineUtil.h"

FAssetRegistry& FAssetRegistry::Get()
{
	static FAssetRegistry Instance;
	return Instance;
}

void FAssetRegistry::RegisterPipeline(FName& Name, UPipeline* Pipeline)
{
	auto It = PipelineMap.find(Name);
	if (It != PipelineMap.end())
	{
		throw EngineUtil::CreateError("UPipeline 등록 실패. 이미 중복된 이름({})이 존재합니다.", Name.ToString());
	}

	PipelineMap.insert({ Name, Pipeline });
}

UPipeline* FAssetRegistry::GetPipeline(FName& Name)
{
	auto It = PipelineMap.find(Name);
	if (It == PipelineMap.end())
	{
		return nullptr;
	}

	return It->second;
}

void FAssetRegistry::ClearPipeline()
{
	PipelineMap.clear();
}

void FAssetRegistry::RegisterStaticMesh(FName& Name, UStaticMesh* StaticMesh)
{
	auto It = StaticMeshMap.find(Name);
	if (It != StaticMeshMap.end())
	{
		throw EngineUtil::CreateError("UStaticMesh 등록 실패. 이미 중복된 이름({})이 존재합니다.", Name.ToString());
	}

	StaticMeshMap.insert({ Name, StaticMesh });
}

UStaticMesh* FAssetRegistry::GetStaticMesh(FName& Name)
{
	auto It = StaticMeshMap.find(Name);
	if (It == StaticMeshMap.end())
	{
		return nullptr;
	}

	return It->second;
}

void FAssetRegistry::ClearStaticMesh()
{
	StaticMeshMap.clear();
}

void FAssetRegistry::RegisterMaterial(FName& Name, UMaterial* Material)
{
	auto It = MaterialMap.find(Name);
	if (It != MaterialMap.end())
	{
		throw EngineUtil::CreateError("UMaterial 등록 실패. 이미 중복된 이름({})이 존재합니다.", Name.ToString());
	}

	MaterialMap.insert({ Name, Material });
}

UMaterial* FAssetRegistry::GetMaterial(FName& Name)
{
	auto It = MaterialMap.find(Name);
	if (It == MaterialMap.end())
	{
		return nullptr;
	}

	return It->second;
}

void FAssetRegistry::ClearMaterial()
{
	MaterialMap.clear();
}

void FAssetRegistry::RegisterFont(FName& Name, UFont* Font)
{
	auto It = FontMap.find(Name);
	if (It != FontMap.end())
	{
		throw EngineUtil::CreateError("UFont 등록 실패. 이미 중복된 이름({})이 존재합니다.", Name.ToString());
	}

	FontMap.insert({ Name, Font });
}

UFont* FAssetRegistry::GetFont(FName& Name)
{
	auto It = FontMap.find(Name);
	if (It == FontMap.end())
	{
		return nullptr;
	}

	return It->second;
}

void FAssetRegistry::ClearFont()
{
	FontMap.clear();
}

void FAssetRegistry::ClearAll()
{
	ClearPipeline();
	ClearStaticMesh();
	ClearMaterial();
	ClearFont();
}
