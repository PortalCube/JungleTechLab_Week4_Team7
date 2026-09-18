#pragma once

#include <cstddef>
#include <format>
#include <stdexcept>
#include <utility>
#include "Runtime/Core/FString.h"

namespace EngineUtil
{

	/// <summary>
	/// 두 해시 값을 하나의 해시 값으로 만듭니다.
	/// </summary>
	/// <param name="FirstHash">해시1</param>
	/// <param name="SecondHash">해시2</param>
	/// <returns>새로 만든 해시값</returns>
	size_t HashCombine(size_t FirstHash, size_t SecondHash);

	template <typename... Args>
	std::runtime_error CreateError(std::format_string<Args...> Format, Args&&... Arguments)
	{
		return std::runtime_error(std::format(Format, std::forward<Args>(Arguments)...));
	}
}
