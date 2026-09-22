#include "FAssetRegistry.h"
#include "Runtime/Utility/EngineUtil.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace
{
	bool IsSubpath(fs::path& OutTargetPath, const fs::path& Parent, const fs::path& Child)
	{
		fs::path ParentNormal = Parent.lexically_normal();
		fs::path ChildNormal = Child.lexically_normal();

		fs::path Relative = ChildNormal.lexically_relative(ParentNormal);

		if (Relative.empty() || *Relative.begin() == ".." || *Relative.begin() == ".") { return false; }
		
		OutTargetPath = *Relative.begin();
		return true;
	}
}

FAssetRegistry& FAssetRegistry::GetInstance()
{
	static FAssetRegistry Instance;
	return Instance;
}

void FAssetRegistry::Register(const FName& Name, UAsset* Pipeline)
{
	auto It = AssetMap.find(Name);
	if (It != AssetMap.end())
	{
		throw EngineUtil::CreateError("UAsset 등록 실패. 이미 중복된 이름({})이 존재합니다.", Name.ToString());
	}

	AssetMap.insert({ Name, Pipeline });
}

void FAssetRegistry::Clear()
{
	AssetMap.clear();
}

FFolderView FAssetRegistry::GetAssetDirectory(const fs::path& ParentPath) const
{
	const auto& Item = DirectoryCache.find(ParentPath);

	if (Item != DirectoryCache.end())
	{
		return Item->second;
	}

	FFolderView Result;

	for (const auto& [AssetID, Asset] : GetAssetMap())
	{
		const FString AssetIDString = AssetID.ToString();
		if (!AssetIDString.empty() && AssetIDString.front() == '#')
		{
			continue;
		}

		fs::path AssetPath{ AssetIDString };
		fs::path TargetPath;

		if (!IsSubpath(TargetPath, ParentPath, AssetPath))
		{
			continue;
		}

		if (TargetPath.has_extension())
		{
			Result.Assets.push_back(Asset);
		}
		else
		{
			Result.Folders.insert(TargetPath);
		}
	}

	DirectoryCache[ParentPath] = Result;

	return Result;
}
