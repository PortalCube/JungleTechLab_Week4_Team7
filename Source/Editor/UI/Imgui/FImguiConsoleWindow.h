#pragma once
#include "Editor/Core/FEditor.h"
#include "Runtime/Core/Log.h"
#include "ThirdParty/Imgui/imgui.h"

class FImguiConsoleWindow final {
public:
	bool bIsOpened = true;
	void Process(FEditor& Editor);
	bool bShowLog = true;
	bool bShowWarn = true;
	bool bShowError = true;
	char InputBuf[256];
	ImVector<const char*> Commands;
	ImVector<char*> History;
	int HistoryPos = -1;    // -1: new line, 0..History.Size-1 browsing history.
	ImGuiTextFilter Filter;
	bool AutoScroll = true;
	bool ScrollToBottom;

	FImguiConsoleWindow();
	~FImguiConsoleWindow();

	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
	{
		FImguiConsoleWindow* console = (FImguiConsoleWindow*)data->UserData;
		return console->TextEditCallback(data);
	}
	int TextEditCallback(ImGuiInputTextCallbackData* data);
	void ExecCommand(const char* command_line);
};