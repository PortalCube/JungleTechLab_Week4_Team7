#pragma once

#include "Runtime/Core/IntTypes.h"

enum class EBlendMode : uint8
{
	Opaque,
	Masked,
	Translucent,
	Additive,
	PremultipliedAlpha,
};

struct FBlendDesc
{
	EBlendMode BlendMode = EBlendMode::Opaque;
};