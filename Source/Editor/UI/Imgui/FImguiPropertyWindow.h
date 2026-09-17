#pragma once
#include "Editor/Core/FEditor.h"

class FImguiPropertyWindow final {
public:
	FImguiPropertyWindow() = default;
	~FImguiPropertyWindow() = default;

	//복사 생성 금지
	FImguiPropertyWindow(const FImguiPropertyWindow&) = delete;
	//복사 대입 금지
	FImguiPropertyWindow& operator=(const FImguiPropertyWindow&) = delete;

	void Process(FEditor& Editor);
};