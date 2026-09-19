#pragma once

#include "Runtime/Core/IntTypes.h"

enum class EDepthWriteMode : uint8
{
	Disable	= 0,
	Enable	= 1,
};

struct FDepthStencilDesc
{
	bool bDepthEnable			= true;
	bool bStencilEnable			= true;
	EDepthWriteMode DepthWrite	= EDepthWriteMode::Enable;
};