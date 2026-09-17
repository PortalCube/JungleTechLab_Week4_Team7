#pragma once
#include "Editor/Core/FEditor.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Input/FCameraInputController.h"
#include "ThirdParty/Imgui/imgui.h"

// 3D 씬 위를 덮는 투명한 ImGui 창.
// - 다른 패널(ControlPanel, Property 등)이 이 창 위에 그려지므로,
//   ImGui 의 hover/active 판정이 "다른 패널에 가려지지 않은 뷰포트 영역"만 걸러준다.
// - 활성 뷰포트의 고정 UV로 창을 배치하고 focus/hover 상태만 갱신한다.
// - 뷰포트 위에서 클릭이 발생하면 피킹을 수행한다.
class FImguiEditorViewportWindow final
{

public:
	FImguiEditorViewportWindow() = default;
	~FImguiEditorViewportWindow() = default;

	//복사 생성 금지
	FImguiEditorViewportWindow(const FImguiEditorViewportWindow&) = delete;
	//복사 대입 금지
	FImguiEditorViewportWindow& operator=(const FImguiEditorViewportWindow&) = delete;


	void Process(FEditor& Editor, float DeltaTime);

private:
	void HandlePicking(FEditor& Editor, const FEditorViewport& Viewport,
		const FVector2& LocalMousePixels, const FVector2& ViewportSizePixels);
	void UpdateGizmoHover(FEditor& Editor, const FEditorViewport& Viewport,
		const FVector2& LocalMousePixels, const FVector2& ViewportSizePixels);

	FCameraInputController CameraController;
};
