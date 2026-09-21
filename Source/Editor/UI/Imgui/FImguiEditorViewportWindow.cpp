#include "FImguiEditorViewportWindow.h"

#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Core/Log.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Actors/AActor.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"

void FImguiEditorViewportWindow::Process(FEditor& Editor, float DeltaTime)
{
    DT = DeltaTime;

    // 종료와 Hover 초기화는 뷰포트의 포커스/표시 여부와 관계없이 처리한다.
    FGizmo& Gizmo = Editor.GetGizmo();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) Gizmo.EndInteraction();
    if (!Gizmo.IsInteracting()) Gizmo.HoveredHandle = EGizmoHandle::None;

    TArray<FEditorViewportClient>& Viewports = Editor.GetViewports();
    FEditorViewportClient* Viewport = Editor.GetActiveViewport();

    const ImGuiViewport* MainViewport = ImGui::GetMainViewport();
    const FVector2 ClientSize{MainViewport->Size.x,MainViewport->Size.y};

    BeginWindow();

    // 부모 창의 콘텐츠 영역
    const ImVec2 ContentPos = ImGui::GetCursorScreenPos();
    const ImVec2 ContentSize = ImGui::GetContentRegionAvail();
    const ImVec2 Origin = MainViewport->Pos;

    if (ClientSize.X <= 0.0f || ClientSize.Y <= 0.0f || ContentSize.x <= 0.0f || ContentSize.y <= 0.0f)
    {
        EndWindow();
        return;
    } 
    // 부모 콘텐츠 영역을 기존 Leaf 좌표계로 변환
    const FRect Rect{
        ContentPos.x - Origin.x,
        ContentPos.y - Origin.y,
        ContentPos.x - Origin.x + ContentSize.x,
        ContentPos.y - Origin.y + ContentSize.y
    };
    Editor.Root->OnResize(Rect);

    // 스플리터 입력 및 Leaf 영역 갱신
    if (Editor.VerticalSplitter.bisActive) ShowViewportVerticalSplitter(Editor.VerticalSplitter);
    if (Editor.HorizonSplitter.bisActive) ShowViewportHorizontalSplitter(Editor.HorizonSplitter);
    if (Editor.HorizonSplitter2.bisActive) {
        Editor.HorizonSplitter2.Ratio = Editor.HorizonSplitter.Ratio;
        ShowViewportHorizontalSplitter(Editor.HorizonSplitter2);
        Editor.HorizonSplitter.Ratio = Editor.HorizonSplitter2.Ratio;
    }

    // 스플리터 비율 저장
    Editor.State.SetSplitter(
        Editor.VerticalSplitter.Ratio,
        Editor.HorizonSplitter.Ratio,
        Editor.HorizonSplitter2.Ratio);

    // 자식 창 안에서 수집하고, 루프 뒤에서 한 번만 처리할 입력
    FViewportInput ActiveInput{};
    bool bHasActiveInput = false;

    for (int i = 0; i < 4; ++i)
    {
        SWindow& leaf = Editor.Leaf[i];
        if (!leaf.bisActive) continue;

        const float Width = leaf.Rect.GetWidth();
        const float Height = leaf.Rect.GetHeight();

        if (Width <= 0.0f || Height <= 0.0f) continue;

        FEditorViewportClient& CurrentViewport =Viewports[leaf.ViewportIndex];

        // Leaf 위치를 ImGui 화면 좌표로 변환
        ImGui::SetCursorScreenPos(ImVec2(
            Origin.x + leaf.Rect.Left,
            Origin.y + leaf.Rect.Top));

        // 자식 창과 내부 UI의 ID를 뷰포트별로 분리
        ImGui::PushID(leaf.ViewportIndex);

        const bool bVisible = ImGui::BeginChild(
            "ViewportChild",
            ImVec2(Width, Height),
            ImGuiChildFlags_None,
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

        if (bVisible)
        {
            //상단바 생성
            DrawViewportHeader();

            //상단바 아래의 실제 3D 영역을 별도 함수로 계산
            FRect SceneRect{};
            if (GetViewportSceneRect(Origin, SceneRect))
            {
                // 상단바를 제외한 영역으로 렌더링·종횡비 설정
                SyncViewportRect(CurrentViewport, SceneRect, ClientSize);
                const FVector2 TopLeftPixels = CurrentViewport.TopLeftUV * ClientSize;
                const FVector2 SizePixels = CurrentViewport.LengthUV * ClientSize;
                FViewportInput Input = GatherInput(TopLeftPixels, SizePixels);

                // 3D 입력 아이템을 누른 경우에만 활성 뷰포트를 변경한다.
                if (Input.bPickRequested || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    Viewport = &CurrentViewport;
                    Editor.ActiveViewportIndex = leaf.ViewportIndex;
                    ImGui::SetWindowFocus();
                    Input.bFocused = true;
                }

                // 클릭 전에도 마우스가 올라간 뷰포트에서 매 프레임 검사한다.
                if (Editor.ObjectSelected() && Input.bHovered && !Gizmo.IsInteracting())
                {
                    UpdateGizmoHover(Editor, CurrentViewport, Input.LocalMouse, Input.SizePixels);
                }

                // 조작은 활성 뷰포트에서 한 번만 처리한다.
                if (&CurrentViewport == Viewport)
                {
                    ActiveInput = Input;
                    bHasActiveInput = true;
                }

                // 각 자식 창 기준 스탯 표시
                if (bOpenMemory) DrawStatsMemory();
                if (bOpenFPS) DrawStatsFPS();
              
            }
        }

        // BeginChild 반환값과 관계없이 반드시 호출
        ImGui::EndChild();
        ImGui::PopID();
    }

    // 활성 뷰포트의 입력을 한 번만 처리
    if (Viewport && Viewport == Editor.GetActiveViewport() && bHasActiveInput)
    {
        Viewport->UpdateFocusedAndHovered(ActiveInput.bFocused,ActiveInput.bHovered);
        UpdateSelection(Editor, *Viewport, ActiveInput);
        UpdateGizmo(Editor, *Viewport, ActiveInput);
        UpdateCamera(Editor, *Viewport, ActiveInput, DeltaTime);
    }
    // 현재 ImGui 창은 다시 부모 창
    ClampWindowToWorkArea();
    EndWindow();
}
void FImguiEditorViewportWindow::BeginWindow() const
{
    constexpr ImGuiWindowFlags WindowFlags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(30.0f, 30.0f));

    ImGui::Begin("Viewport", nullptr, WindowFlags);

    // 3D 는 이 창 아래에 그려지므로 창 자체는 항상 가장 뒤에 둔다.
    ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindow());

    ImGui::PopStyleVar(3);
}

void FImguiEditorViewportWindow::EndWindow() const
{
    ImGui::End();
}

void FImguiEditorViewportWindow::SyncViewportRect(FEditorViewportClient &Viewport, const FRect& Rect,
                                                  const FVector2 &ClientSize) const
{
    const FVector2 WindowPos{ImGui::GetWindowPos().x, ImGui::GetWindowPos().y};
    const FVector2 WindowSize{ImGui::GetWindowSize().x, ImGui::GetWindowSize().y};

    // 창을 접거나 탭으로 숨기면 0 이 될 수 있으므로 나눗셈 전에 막는다.
    if (WindowSize.X <= 0.0f || WindowSize.Y <= 0.0f)
    {
        return;
    }

    Viewport.ViewportCamera.Projection.Aspect = Rect.GetWidth() / Rect.GetHeight();

    // 픽셀 -> 0~1 비율. 창 크기가 바뀌어도 이 값은 그대로 쓸 수 있다.
    Viewport.TopLeftUV = FVector2{Rect.Left / ClientSize.X, Rect.Top / ClientSize.Y};
    Viewport.LengthUV = FVector2{Rect.GetWidth() / ClientSize.X, Rect.GetHeight() / ClientSize.Y};
}

FImguiEditorViewportWindow::FViewportInput FImguiEditorViewportWindow::GatherInput(const FVector2& ViewportTopLeftPixels, const FVector2& ViewportSizePixels) const
{
    // 상단바 아래 3D 영역만 등록한다. 드래그 중에는 영역 밖에서도 활성 상태를 유지한다.
    ImGui::InvisibleButton("ViewportInput", ImVec2(ViewportSizePixels.X, ViewportSizePixels.Y), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    FViewportInput Input;
    Input.SizePixels = ViewportSizePixels;
    Input.LocalMouse = FInputManager::Get().GetMousePosition() - ViewportTopLeftPixels;
    Input.bHovered = ImGui::IsItemHovered();
    Input.bFocused = ImGui::IsWindowFocused();
    Input.bPickRequested = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    Input.bLeftDown = ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    Input.bLeftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

    return Input;
}

void FImguiEditorViewportWindow::ClampWindowToWorkArea() const
{
    const float WorkTop = ImGui::GetMainViewport()->WorkPos.y;
    const ImVec2 WindowPos = ImGui::GetWindowPos();

    if (WindowPos.y < WorkTop)
    {
        ImGui::SetWindowPos(ImVec2(WindowPos.x, WorkTop));
    }
}

void FImguiEditorViewportWindow::UpdateSelection(FEditor &Editor,
                                                 const FEditorViewportClient &Viewport,
                                                 const FViewportInput &Input)
{
    if (Input.bPickRequested)
    {
        HandlePicking(Editor, Viewport, Input.LocalMouse, Input.SizePixels);
    }
}

void FImguiEditorViewportWindow::UpdateGizmo(FEditor& Editor, const FEditorViewportClient& Viewport, const FViewportInput& Input)
{
    FGizmo& Gizmo = Editor.GetGizmo();

    // Hover와 종료는 Process에서 처리하고, 여기서는 진행 중인 드래그만 갱신한다.
    if (Editor.ObjectSelected() && Gizmo.IsInteracting() && Input.bLeftDown)
    {
        Gizmo.UpdateInteraction(Editor, Input.LocalMouse);
    }
}

void FImguiEditorViewportWindow::UpdateCamera(FEditor &Editor, FEditorViewportClient &Viewport,
                                              const FViewportInput &Input, float DeltaTime)
{
    if (!Input.bFocused)
    {
        return;
    }

    CameraController.CameraRotateSpeed = Editor.State.GetCameraSensitivity();
    CameraController.CameraMoveSpeed = Editor.State.GetCameraSpeed();

    FCamera &Camera = Viewport.ViewportCamera;
    CameraController.UpdateMouseInput(Camera);

    // 우클릭 중에는 WASD 가 카메라 비행에 쓰이므로 단축키와 겹치지 않게 나눈다.
    if (FInputManager::Get().IsMouseDown(EMouseButton::Right))
    {
        CameraController.UpdateKeyInput(Camera, DeltaTime);
        return;
    }


    // 기즈모를 드래그하는 중에는 모드가 바뀌면 안 된다.
    if (!Editor.GetGizmo().IsInteracting())
    {
        UpdateShortcuts(Editor);
    }
}

void FImguiEditorViewportWindow::UpdateShortcuts(FEditor &Editor) const
{
    FInputManager &Input = FInputManager::Get();
    FGizmo &Gizmo = Editor.GetGizmo();

    // 백틱(`) : 월드/로컬 공간 전환.
    // Translate/Rotate 에서만 의미가 있어 None/Scale 은 제외한다.
    if (Input.IsKeyJustPressed(VK_OEM_3))
    {
        if (Gizmo.Mode != EGizmoMode::None && Gizmo.Mode != EGizmoMode::Scale)
        {
            Gizmo.SetGizmoSpace(
                static_cast<EGizmoSpace>((static_cast<uint8>(Gizmo.GetSpace()) + 1) % 2));
        }
    }

    if (Input.IsKeyJustPressed('Q'))
    {
        Gizmo.Mode = EGizmoMode::None;
    }
    else if (Input.IsKeyJustPressed('W'))
    {
        Gizmo.Mode = EGizmoMode::Translate;
    }
    else if (Input.IsKeyJustPressed('E'))
    {
        Gizmo.Mode = EGizmoMode::Rotate;
    }
    else if (Input.IsKeyJustPressed('R'))
    {
        Gizmo.Mode = EGizmoMode::Scale;
    }
    else if (Input.IsKeyJustPressed(VK_SPACE))
    {
        Gizmo.Mode = static_cast<EGizmoMode>((static_cast<uint8>(Gizmo.Mode) + 1) % 4);
    }

}

void FImguiEditorViewportWindow::HandlePicking(FEditor &Editor,
                                               const FEditorViewportClient &Viewport,
                                               const FVector2 &LocalMousePixels,
                                               const FVector2 &ViewportSizePixels)
{
    // 기즈모 핸들 위를 눌렀으면 피킹 대신 조작을 시작한다.
    if (Editor.GetSelectedActor() != nullptr)
    {
        // Process에서 현재 뷰포트의 Hover 판정을 먼저 갱신한 상태다.
        FGizmo& Gizmo = Editor.GetGizmo();

        if (Gizmo.HoveredHandle != EGizmoHandle::None)
        {
            Gizmo.BeginInteraction(
                Editor.SelectedTransform,
                Gizmo.HoveredHandle,
                LocalMousePixels,
                Viewport.ViewportCamera,
                ViewportSizePixels);

            return;
        }
    }
    TArray<UPrimitiveComponent *> Components = Editor.GetPrimitiveComponents();

    UPrimitiveComponent *HitComponent = nullptr;
    FVector ImpactPoint;

    const bool bHit = FRayCastingManager::RayIntersectsMeshes(
        FRayCastingManager::CreateRayFromScreenPosition(
            Viewport.ViewportCamera, LocalMousePixels, ViewportSizePixels),
            Viewport.ViewportCamera,
        Components, HitComponent, ImpactPoint);

    // 피킹은 액터 단위로 선택한다. 소유 액터가 없으면 선택할 수 없다.
    if (!bHit || !HitComponent || !HitComponent->GetActorOwner())
    {
        Editor.UnSelectActor();
        return;
    }

    AActor *OwnerActor = HitComponent->GetActorOwner();
    Editor.SelectActor(OwnerActor);

    const char *ActorClass =
        OwnerActor->GetClass() ? OwnerActor->GetClass()->GetDisplayName().c_str() : "Unknown";
    const char *CompClass =
        HitComponent->GetClass() ? HitComponent->GetClass()->GetDisplayName().c_str() : "Unknown";

    UE_LOG("[Picking] Actor: %s (UUID: %u), Component: %s (UUID: %u)", ActorClass,
           OwnerActor->GetUUID(), CompClass, HitComponent->GetUUID());
}

void FImguiEditorViewportWindow::UpdateGizmoHover(FEditor &Editor,
                                                  const FEditorViewportClient &Viewport,
                                                  const FVector2 &LocalMousePixels,
                                                  const FVector2 &ViewportSizePixels)
{
    FRay Ray = FRayCastingManager::CreateRayFromScreenPosition(
        Viewport.ViewportCamera, LocalMousePixels, ViewportSizePixels);

    FGizmo &Gizmo = Editor.GetGizmo();
    Gizmo.HoveredHandle = Gizmo.HitTest(Editor.SelectedTransform, Ray, Viewport.ViewportCamera);
}

void FImguiEditorViewportWindow::ShowViewportVerticalSplitter(SSplitter& Splitter)
{   //SplitterV용
    const FRect& R = Splitter.Rect;
    const ImVec2 Origin = ImGui::GetMainViewport()->Pos;

    float Top = R.GetHeight() * Splitter.Ratio;
    float Bottom = R.GetHeight() - Top;
    const float Y = Origin.y + R.Top + Top;

    ImGui::PushID(&Splitter);

    const ImVec4 Color = ImGui::GetStyleColorVec4(ImGuiCol_Separator);
    ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, Color);
    ImGui::PushStyleColor(ImGuiCol_SeparatorActive, Color);

    ImGui::SplitterBehavior(
        ImRect(ImVec2(Origin.x + R.Left, Y - 3),
            ImVec2(Origin.x + R.Right, Y + 3)),
        ImGui::GetID("VerticalSplitter"),
        ImGuiAxis_Y,
        &Top, &Bottom,
        10.0f, 10.0f);

    ImGui::PopStyleColor(2);
    ImGui::PopID();

    Splitter.Ratio = Top / R.GetHeight();
    Splitter.OnResize(R);
}
void FImguiEditorViewportWindow::ShowViewportHorizontalSplitter(SSplitter& Splitter)
{
    const FRect& R = Splitter.Rect;
    const ImVec2 Origin = ImGui::GetMainViewport()->Pos;

    float Left = R.GetWidth() * Splitter.Ratio;
    float Right = R.GetWidth() - Left;
    const float X = Origin.x + R.Left + Left;
    ImGui::PushID(&Splitter);

    const ImVec4 Color = ImGui::GetStyleColorVec4(ImGuiCol_Separator);

    ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, Color);
    ImGui::PushStyleColor(ImGuiCol_SeparatorActive, Color);

    ImGui::SplitterBehavior(
        ImRect(ImVec2(X - 3, Origin.y + R.Top),
            ImVec2(X + 3, Origin.y + R.Bottom)),
        ImGui::GetID("HorizontalSplitter"),
        ImGuiAxis_X,
        &Left, &Right,
        10.0f, 10.0f);

    ImGui::PopStyleColor(2);
    ImGui::PopID();

    Splitter.Ratio = Left / R.GetWidth();
    Splitter.OnResize(R);
}

void FImguiEditorViewportWindow::DrawStatLine(ImDrawList* DrawList, const ImVec2& Position,
    float& Y, const char* Name, const char* Value, FVector4 Color)
{
    const float ValueOffsetX = 240.0f;
    const ImU32 TextColor = IM_COL32(Color.X, Color.Y, Color.Z, Color.W);

    if (Name[0] != '\0'){
        DrawList->AddText(ImVec2(Position.x, Y), TextColor, Name);
        DrawList->AddText(ImVec2(Position.x + ValueOffsetX, Y), TextColor, Value);
    }
    else{
        DrawList->AddText(ImVec2(Position.x, Y), TextColor, Value);
    }

    Y += 20.0f;
}

void FImguiEditorViewportWindow::DrawRow(ImDrawList* DrawList, const ImVec2& Pos,
    float& Y, const char* Str, double Data, float RowColor)
{
    const float Width = 500.0f;
    const float RowHeight = 20.0f;
    FVector4 Color(255.0f, 255.0f, 255.0f, 255.0f);
    char Buffer[64];

    // Memory 전체 배경
    DrawList->AddRectFilled(
        ImVec2(Pos.x, Y),
        ImVec2(Pos.x + Width, Y + RowHeight),
        IM_COL32(RowColor, RowColor, RowColor, 200)
    );

    // Vertex Shader
    sprintf_s(Buffer, "%.2f MB", Data);
    DrawStatLine(DrawList, Pos, Y, Str, Buffer, Color);
}

void FImguiEditorViewportWindow::DrawStatsMemory()
{
    ImVec2 ViewportPos = ImGui::GetWindowPos();
    ImVec2 ViewportSize = ImGui::GetWindowSize();

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 Pos = {
        ViewportPos.x + ViewportSize.x * 0.2f,
        ViewportPos.y + ViewportSize.y * 0.2f
    };
    
    float Y = Pos.y;

    DrawList->AddText(ImVec2(Pos.x, Pos.y - 45.0f), IM_COL32(255, 255, 255, 255), "Memory");
    DrawList->AddText(ImVec2(Pos.x, Pos.y - 20.0f), IM_COL32(255, 165, 0, 255), "Memory Counters");
    DrawList->AddText(ImVec2(Pos.x + 240.0f, Pos.y - 20.0f), IM_COL32(255, 165, 0, 255), "UsedMax");
    // CPU
    DrawRow(DrawList, Pos, Y, "CPU Memory",
        static_cast<double>(FStatsManager::Get().GetProcessMemoryUsed())
        / (1024.0 * 1024.0), 30.0f);
    // Ram
    DrawRow(DrawList, Pos, Y, "Ram Used",
        static_cast<double>(FStatsManager::Get().GetSystemMemoryUsed())
        / (1024.0 * 1024.0 * 1024.0), 10.0f);
    // Ram Available
    DrawRow(DrawList, Pos, Y, "Ram Available",
        static_cast<double>(FStatsManager::Get().GetSystemMemoryAvailable())
        / (1024.0 * 1024.0 * 1024.0), 30.0f);
    // GPU
    DrawRow(DrawList, Pos, Y, "GPU Memory Used",
        static_cast<double>(FStatsManager::Get().GetGPUMemoryUsed())
        / (1024.0 * 1024.0 * 1024.0), 10.0f);
    // GPU Available
    DrawRow(DrawList, Pos, Y, "GPU Memory Available",
        static_cast<double>(FStatsManager::Get().GetGPUMemoryBudget())
        / (1024.0 * 1024.0), 30.0f);

    // Vetex Shader
    DrawRow(DrawList, Pos, Y, "VertexShader", 
        static_cast<double>(FStatsManager::Get().GetVertexShaderMemoryUsed())
        / (1024.0 * 1024.0), 10.0f);
    // Pixel Shader
    DrawRow(DrawList, Pos, Y, "Pixel Shader",
        static_cast<double>(FStatsManager::Get().GetPixelShaderMemoryUsed())
        / (1024.0 * 1024.0), 30.0f);
    // Texture
    DrawRow(DrawList, Pos, Y, "Texture",
        static_cast<double>(FStatsManager::Get().GetTextureMemoryUsed())
        / (1024.0 * 1024.0), 10.0f);

    DrawRow(DrawList, Pos, Y, "Total Memory Pool",
        static_cast<double>(FStatsManager::Get().GetMemoryPool()) //, 30.0f);
        / (1024.0 * 1024.0), 30.0f);

    DrawRow(DrawList, Pos, Y, "Memory Pool Used",
        static_cast<double>(FStatsManager::Get().GetMemoryPoolUsed()) //, 10.0f);
        / (1024.0 * 1024.0), 10.0f);

    DrawRow(DrawList, Pos, Y, "Memory Pool Free",
        static_cast<double>(FStatsManager::Get().GetMemoryPoolFree()) //, 30.0f);
        / (1024.0 * 1024.0), 30.0f);

}

void FImguiEditorViewportWindow::DrawStatsFPS()
{
    ImVec2 ViewportPos = ImGui::GetWindowPos();
    ImVec2 ViewportSize = ImGui::GetWindowSize();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 FPSPos = {
        ViewportPos.x + ViewportSize.x - 180.0f,
        ViewportPos.y + ViewportSize.y * 0.25f
    };

    float Y = FPSPos.y;
    FVector4 FPSColor(0.0f, 255.0f, 255.0f, 255.0f);
    char Buffer[64];

    sprintf_s(Buffer, "%.2f FPS", 1.0f / DT);
    DrawStatLine(DrawList, FPSPos, Y, "", Buffer, FPSColor);
    sprintf_s( Buffer, "%.2f ms", DT * 1000.0f);
    DrawStatLine( DrawList, FPSPos, Y, "", Buffer, FPSColor);
}

bool FImguiEditorViewportWindow::GetViewportSceneRect(
    const ImVec2& Origin,
    FRect& OutRect) const
{
    const ImVec2 ScenePos = ImGui::GetCursorScreenPos();
    const ImVec2 SceneSize = ImGui::GetContentRegionAvail();

    if (SceneSize.x <= 0.0f || SceneSize.y <= 0.0f)
    {
        return false;
    }

    OutRect = FRect{
        ScenePos.x - Origin.x,
        ScenePos.y - Origin.y,
        ScenePos.x - Origin.x + SceneSize.x,
        ScenePos.y - Origin.y + SceneSize.y
    };

    return true;
}

void FImguiEditorViewportWindow::DrawViewportHeader() const
{
    const float HeaderHeight = ImGui::GetFrameHeight();
    const float ButtonSize = HeaderHeight - 6.0f;

    const ImVec4 HeaderColor{ 0.16f, 0.29f, 0.48f, 1.0f };
    const ImVec4 HoverColor{ 0.24f, 0.42f, 0.65f, 1.0f };
    const ImVec4 ActiveColor{ 0.30f, 0.50f, 0.76f, 1.0f };
    const ImVec4 BorderColor{ 0.42f, 0.62f, 0.85f, 1.0f };
    const ImVec4 HighlightColor{ 0.72f, 0.86f, 1.0f, 1.0f };

    ImGui::PushStyleColor(ImGuiCol_ChildBg, HeaderColor);
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, HeaderColor);
    ImGui::PushStyleColor(ImGuiCol_Button, HeaderColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HoverColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ActiveColor);
    ImGui::PushStyleColor(ImGuiCol_Border, BorderColor);
    ImGui::PushStyleColor(ImGuiCol_Header, HoverColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HoverColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ActiveColor);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(8.0f, 0.0f));
    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

    const bool bVisible = ImGui::BeginChild(
        "ViewportHeader",
        ImVec2(0.0f, HeaderHeight),
        ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    if (bVisible && ImGui::BeginMenuBar())
    {
        ImGui::TextUnformatted("Viewport");

        const ImGuiStyle& Style = ImGui::GetStyle();
        ImDrawList* HeaderDrawList = ImGui::GetWindowDrawList();

        // 오른쪽 끝의 최대화 버튼 위치
        const float ButtonX =ImGui::GetWindowWidth() -Style.WindowPadding.x -ButtonSize;

        // 최대화 버튼 왼쪽의 Camera 메뉴 위치
        const float CameraWidth =ImGui::CalcTextSize("Camera").x +Style.ItemSpacing.x * 3.0f;
        const float CameraX = ButtonX - CameraWidth;

        if (CameraX > ImGui::GetCursorPosX()) ImGui::SetCursorPosX(CameraX);

        const bool bCameraOpen = ImGui::BeginMenu("Camera");

        // 팝업 내용을 제출하기 전에 메뉴 버튼 정보를 보관
        const ImVec2 CameraMin = ImGui::GetItemRectMin();
        const ImVec2 CameraMax = ImGui::GetItemRectMax();
        const bool bCameraHovered = ImGui::IsItemHovered();

        HeaderDrawList->AddRect(
            ImVec2(CameraMin.x + 0.5f, CameraMin.y + 0.5f),
            ImVec2(CameraMax.x - 0.5f, CameraMax.y - 0.5f),
            ImGui::GetColorU32(
                (bCameraOpen || bCameraHovered)
                ? HighlightColor
                : BorderColor),
            3.0f,
            0,
            1.0f);

        if (bCameraOpen)
        {
            ImGui::TextUnformatted("Perspective");
            ImGui::Separator();
            ImGui::MenuItem("Top");
            ImGui::MenuItem("Bottom");
            ImGui::MenuItem("Left");
            ImGui::MenuItem("Right");
            ImGui::MenuItem("Front");
            ImGui::MenuItem("Back");

            ImGui::EndMenu();
        }

        // 최대화 버튼 오른쪽 정렬
        if (ButtonX > ImGui::GetCursorPosX()) ImGui::SetCursorPosX(ButtonX);


        // 줄어든 버튼을 상단바의 세로 중앙에 배치
        ImGui::SetCursorPosY(
            (HeaderHeight - ButtonSize) * 0.5f);

        // UI만 표시
        ImGui::Button("##Maximize",ImVec2(ButtonSize, ButtonSize));

        const ImVec2 ButtonMin = ImGui::GetItemRectMin();
        const ImVec2 ButtonMax = ImGui::GetItemRectMax();
        const bool bButtonHovered = ImGui::IsItemHovered();

        // 버튼 내부의 최대화 아이콘
        const float IconPadding = ButtonSize * 0.25f;
        if (bButtonHovered)
        {
            // 호버 시 바깥 테두리 강조
            HeaderDrawList->AddRect(
                ImVec2(ButtonMin.x + 0.5f, ButtonMin.y + 0.5f),
                ImVec2(ButtonMax.x - 0.5f, ButtonMax.y - 0.5f),
                ImGui::GetColorU32(HighlightColor),3.0f,0, 1.0f);
            ImGui::SetTooltip("Maximize / Restore");
        }

        ImGui::EndMenuBar();
    }

    ImGui::EndChild();

    ImGui::PopStyleVar(5);
    ImGui::PopStyleColor(9);
}
