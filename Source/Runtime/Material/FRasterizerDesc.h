#pragma once

#include "Runtime/Core/IntTypes.h"

enum class ERasterizerFillMode : uint8
{
	Solid		= 0,
	Wireframe	= 1,
};

enum class ERasterizerCullMode : uint8
{
	None	= 0,
	Front	= 1,
	Back	= 2,
};

enum class ERasterizerFrontFaceMode : uint8
{
	CounterClockwise	= 0,
	Clockwise			= 1,
};

struct FRasterizerDesc
{
	ERasterizerFillMode FillMode		= ERasterizerFillMode::Solid;
	ERasterizerCullMode CullMode		= ERasterizerCullMode::Back;
	ERasterizerFrontFaceMode FrontFace	= ERasterizerFrontFaceMode::CounterClockwise;
	bool bUseMultisample				= false;
	bool bUseAntialiasedLine			= false;
};