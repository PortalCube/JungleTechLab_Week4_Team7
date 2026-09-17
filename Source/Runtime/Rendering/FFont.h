#pragma once

#include "Runtime/Core/TMap.h"
#include "ThirdParty/Json/json.hpp"
#include "Runtime/Core/FString.h"

struct FCharacterInfo
{
	float u;
	float v;
	float width;
	float height;

	// 가변폭
	float advance;
	float planeLeft;
	float planeTop;
	float planeRight;
	float planeBottom;
};

class FFont
{
public:
	void InitializeForASCII(float InNumberOfLine);
	void Deserialize(const FWString& path);
	const FCharacterInfo& GetCharInfo(char32_t InCharacter) const;
private:
	//FTexture	// 텍스처 아틀라스
	//FMeterial	// 폰트 머터리얼

	TMap<char32_t, FCharacterInfo> CharInfoMap;
};