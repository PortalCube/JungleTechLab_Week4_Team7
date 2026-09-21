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
void FImguiEditorViewportWindow::Process(FEditor &Editor, float DeltaTime)
{
    DT = DeltaTime;
    TArray<FEditorViewportClient>& Viewports = Editor.GetViewports();
    FEditorViewportClient* Viewport=nullptr;
         //TArray<FEditorViewportClient>& Viewports = Editor.GetActiveViewport();
        //FEditorViewportClient& Viewport = Viewports[i];


        const ImGuiViewport* MainViewport = ImGui::GetMainViewport();

        const FVector2 ClientSize{ MainViewport->Size.x, MainViewport->Size.y };
        
        //마우스정보
        BeginWindow();
        const FVector2 Mouse = FInputManager::Get().GetMousePosition();
        const bool bWindowHovered = ImGui::IsWindowHovered();


        // 창의 현재 사각형을 FRect로 변환해서 Root에 넘겨준다.
        // Imgui의 전체3D화면 기준 Rect
        const ImVec2 ContentPos = ImGui::GetCursorScreenPos(); // 3D창의 좌상단
        const ImVec2 ContentSize = ImGui::GetContentRegionAvail(); // 3D창의 Width,Height
        const ImVec2 Origin = MainViewport->Pos;
        FRect Rect = {
        ContentPos.x - Origin.x,
        ContentPos.y - Origin.y,
        ContentPos.x - Origin.x + ContentSize.x,
        ContentPos.y - Origin.y + ContentSize.y };
        Editor.Root->OnResize(Rect);

        // 각 Leaf마다 알맞게 전달해준다.
        if (Editor.VerticalSplitter.bisActive) ShowViewportVerticalSplitter(Editor.VerticalSplitter);
        if (Editor.HorizonSplitter.bisActive) ShowViewportHorizontalSplitter(Editor.HorizonSplitter);
        if (Editor.HorizonSplitter2.bisActive)
        {
            Editor.HorizonSplitter2.Ratio = Editor.HorizonSplitter.Ratio;
            ShowViewportHorizontalSplitter(Editor.HorizonSplitter2);
            Editor.HorizonSplitter.Ratio = Editor.HorizonSplitter2.Ratio;
        }


    // 스탯 창

         //CurrentViewport와 Leaf를 일치화시킨다.(Active일때만)
        for (int i = 0;i < 4;i++)
        {
            SWindow& leaf = Editor.Leaf[i];
            if (!leaf.bisActive) continue;

            FEditorViewportClient& CurrentViewport = Viewports[leaf.ViewportIndex];
            SyncViewportRect(CurrentViewport, leaf.Rect, ClientSize);

            const FVector2 TopLeftPixels = CurrentViewport.TopLeftUV * ClientSize;
            const FVector2 SizePixels = CurrentViewport.LengthUV * ClientSize;
     
            
            //마우스가 focus된 viewport 구분
            if (bWindowHovered &&
                Mouse.X >= leaf.Rect.Left && Mouse.X < leaf.Rect.Right &&
                Mouse.Y >= leaf.Rect.Top && Mouse.Y < leaf.Rect.Bottom)
            {
                Viewport = &CurrentViewport;

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
                    ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    Editor.ActiveViewportIndex = leaf.ViewportIndex;
                }
            }
        }

        //마우스가 focus된 viewport 처리
        if (Viewport && Viewport == Editor.GetActiveViewport())
        {
            const FVector2 TopLeftPixels = Viewport->TopLeftUV * ClientSize;
            const FVector2 SizePixels = Viewport->LengthUV * ClientSize;
            const FViewportInput Input = GatherInput(TopLeftPixels, SizePixels);
            Viewport->UpdateFocusedAndHovered(Input.bFocused, Input.bHovered);

            UpdateGizmo(Editor, *Viewport, Input);
            UpdateSelection(Editor, *Viewport, Input);
            UpdateCamera(Editor, *Viewport, Input, DeltaTime);
        }

        ClampWindowToWorkArea();

        if (bOpenMemory) DrawStatsMemory();
        if (bOpenFPS) DrawStatsFPS();

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

    ImGui::Begin("ViewPort", nullptr, WindowFlags);

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

FImguiEditorViewportWindow::FViewportInput
FImguiEditorViewportWindow::GatherInput(const FVector2 &ViewportTopLeftPixels,
                                        const FVector2 &ViewportSizePixels) const
{
    // 뷰포트 영역 전체를 덮는 클릭 판정용 아이템.
    // 다른 ImGui 창이 위에 있으면 IsItemHovered()/IsItemClicked() 가 false 가
    // 되어 자연스럽게 focus 중재가 된다.

    FViewportInput Input;
    Input.SizePixels = ViewportSizePixels;
    Input.LocalMouse =
        FInputManager::Get().GetMousePosition() - ViewportTopLeftPixels;

    // Process에서 창 Hover와 Leaf 영역 판정을 마친 상태
    Input.bHovered = true;
    Input.bFocused = ImGui::IsWindowFocused();
    Input.bPickRequested = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    Input.bLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
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

void FImguiEditorViewportWindow::UpdateGizmo(FEditor &Editor,
                                             const FEditorViewportClient &Viewport,
                                             const FViewportInput &Input)
{
    FGizmo &Gizmo = Editor.GetGizmo();

    if (Input.bLeftDown)
    {
        Gizmo.UpdateInteraction(Editor, Input.LocalMouse);
    }

    if (Input.bLeftReleased)
    {
        Gizmo.EndInteraction();
    }

    if (Input.bHovered)
    {
        UpdateGizmoHover(Editor, Viewport, Input.LocalMouse, Input.SizePixels);
    }
    else
    {
        Gizmo.HoveredHandle = EGizmoHandle::None;
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
        FGizmo &Gizmo = Editor.GetGizmo();
        if (Gizmo.HoveredHandle != EGizmoHandle::None)
        {
            Gizmo.BeginInteraction(Editor.SelectedTransform, Gizmo.HoveredHandle,
                                   LocalMousePixels, Viewport.ViewportCamera,
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
