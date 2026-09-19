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

        // 각 Leaf마다 알맞게 전달해준다.
        Editor.Root->OnResize(Rect);


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
