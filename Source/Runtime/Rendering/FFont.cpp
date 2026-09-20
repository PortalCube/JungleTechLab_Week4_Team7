#include "FFont.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/FString.h"
#include "ThirdParty/Json/json.hpp"
#include <filesystem>
#include <fstream>

void FFont::InitializeForASCII(float InNumberOfLine)
{
	// 16x16 코드페이지 437 기준
	float uvSize = 1.0f / InNumberOfLine;
	for (uint16 i = 0; i < 256; ++i)
	{
		uint16 col = i % 16;
		uint16 row = i / 16;

		FCharacterInfo ci;
		ci.u = col * uvSize;
		ci.v = row * uvSize;
		ci.width = uvSize;
		ci.height = uvSize;

		CharInfoMap[static_cast<char>(i)] = ci;
	}
}

void FFont::Deserialize(const FWString& path)
{
	std::ifstream f(path);
	if (!f)
	{
		return;
	}

	nlohmann::json data;
	f >> data;

	// atlas 자체 정보 
	FString type = data["atlas"]["type"].get<std::string>();
	uint32 distanceRange = data["atlas"]["distanceRange"].get<uint32>();
	uint32 dixtanceRangeMiddle = data["atlas"]["distanceRangeMiddle"].get<uint32>();
	uint32 size = data["atlas"]["size"].get<uint32>();
	uint32 width = data["atlas"]["width"].get<uint32>();
	uint32 height = data["atlas"]["height"].get<uint32>();
	FString yOrigin = data["atlas"]["yOrigin"].get<std::string>();

	// metrics
	uint32 emSize = data["metrics"]["emSize"].get<uint32>();
	float lineHeight = data["metrics"]["lineHeight"].get<float>();
	float ascender = data["metrics"]["ascender"].get<float>();
	float descender = data["metrics"]["descender"].get<float>();
	float underlineY = data["metrics"]["underlineY"].get<float>();
	float underlineThickness = data["metrics"]["underlineThickness"].get<float>();

	// 문자
	for (const auto& glyph : data["glyphs"])
	{
		FCharacterInfo info{};

		uint32 unicode = glyph["unicode"].get<uint32>();
		info.advance = glyph["advance"].get<float>();

		if (glyph.contains("planeBounds"))
		{
			info.planeLeft = glyph["planeBounds"]["left"].get<float>();
			info.planeTop = glyph["planeBounds"]["top"].get<float>();
			info.planeRight = glyph["planeBounds"]["right"].get<float>();
			info.planeBottom = glyph["planeBounds"]["bottom"].get<float>();
		}

		if (glyph.contains("atlasBounds"))
		{
			float atlLeft = glyph["atlasBounds"]["left"].get<float>();
			float atlTop = glyph["atlasBounds"]["top"].get<float>();
			float atlRight = glyph["atlasBounds"]["right"].get<float>();
			float atlBot = glyph["atlasBounds"]["bottom"].get<float>();


			info.u = atlLeft / width;
			info.v = atlTop / height;
			info.width = (atlRight - atlLeft) / width;
			info.height = (atlBot - atlTop) / height;
		}

		CharInfoMap.emplace(static_cast<char32_t>(unicode), info);
	}
}

const FCharacterInfo& FFont::GetCharInfo(char32_t InCharacter) const
{
	auto it = CharInfoMap.find(InCharacter);
	if (it != CharInfoMap.end())
	{
		return it->second;
	}

	auto fallbackIt = CharInfoMap.find('?');
	if (fallbackIt != CharInfoMap.end())
	{
		return fallbackIt->second;
	}
		
	static const FCharacterInfo defaultInfo{};
	return defaultInfo;
}

void FFont::SetTexture(const TSharedPtr<FTexture>& InTexture)
{
	Texture = InTexture;
}
