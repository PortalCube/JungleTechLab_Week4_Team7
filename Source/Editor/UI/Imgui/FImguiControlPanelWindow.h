#pragma once
#include "Editor/Core/FEditor.h"

class FImguiControlPanelWindow final {
public:
	FImguiControlPanelWindow() = default;
	~FImguiControlPanelWindow() = default;

	//복사 생성 금지
	FImguiControlPanelWindow(const FImguiControlPanelWindow&) = delete;
	//복사 대입 금지
	FImguiControlPanelWindow& operator=(const FImguiControlPanelWindow&) = delete;

	void Process(FEditor& Editor);

};
