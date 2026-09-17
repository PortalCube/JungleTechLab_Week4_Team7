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

// 뷰포트를 덮는 투명한 창
void FImguiEditorViewportWindow::Process(FEditor &Editor, float DeltaTime) {
  const ImGuiViewport *MainViewport = ImGui::GetMainViewport();
  FEditorViewport *Viewport = Editor.GetActiveViewport();
  if (!Viewport)
    return;

  const FVector2 ClientSize = {MainViewport->Size.x, MainViewport->Size.y};
  const FVector2 ViewportTopLeftPixels = Viewport->TopLeftUV * ClientSize;
  const FVector2 ViewportSizePixels = Viewport->LengthUV * ClientSize;
  const ImVec2 WorkPos = MainViewport->WorkPos;

  // ImGui::SetNextWindowPos(ImVec2(MainViewport->Pos.x +
  // ViewportTopLeftPixels.X, 	MainViewport->Pos.y + ViewportTopLeftPixels.Y));
  // ImGui::SetNextWindowSize(ImVec2(ViewportSizePixels.X,
  // ViewportSizePixels.Y));

  constexpr ImGuiWindowFlags WindowFlags =
      //	ImGuiWindowFlags_NoTitleBar |
      //	ImGuiWindowFlags_NoResize |
      //	ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground |
      //	ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  // ImGui::SetNextWindowBgAlpha(0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(30.0f, 30.0f));

  ImGui::Begin("Viewport", nullptr, WindowFlags);
  // ImGui::Begin("##EditorViewport", nullptr);
  ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindow());

  ImGui::PopStyleVar(3);

  FVector2 WindowPos = {ImGui::GetWindowPos().x, ImGui::GetWindowPos().y};
  FVector2 WindowSize = {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y};

  // 뷰포트 영역 전체를 덮는 클릭 판정용 아이템.
  // 다른 ImGui 창이 위에 있으면 IsItemHovered()/IsItemClicked() 가 false 가
  // 되어 자연스럽게 focus 중재가 된다.
  ImGui::InvisibleButton(
      "##ViewportInput", ImVec2(ViewportSizePixels.X, ViewportSizePixels.Y),
      ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

  Viewport->ViewportCamera.Projection.Aspect = WindowSize.X / WindowSize.Y;
  WindowPos.X /= ClientSize.X;
  WindowPos.Y /= ClientSize.Y;
  WindowSize.X /= ClientSize.X;
  WindowSize.Y /= ClientSize.Y;
  Viewport->TopLeftUV = WindowPos;
  Viewport->LengthUV = WindowSize;

  const bool bHovered = ImGui::IsItemHovered();
  const bool bFocused = ImGui::IsWindowFocused();

  const bool bPickRequested = ImGui::IsItemClicked(ImGuiMouseButton_Left);
  const bool bLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
  const bool bLeftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

  if (FEditorViewport *ActiveViewport = Editor.GetActiveViewport()) {
    ActiveViewport->UpdateFocusedAndHovered(bFocused, bHovered);

    const FVector2 LocalMouse =
        FInputManager::Get().GetMousePosition() - ViewportTopLeftPixels;

    FGizmo &Gizmo = Editor.GetGizmo();

    if (bPickRequested) {
      HandlePicking(Editor, *ActiveViewport, LocalMouse, ViewportSizePixels);
    }

    if (bLeftDown) {
      Gizmo.UpdateInteraction(Editor, LocalMouse);
    }

    if (bLeftReleased) {
      Gizmo.EndInteraction();
    }

    if (bHovered) {
      UpdateGizmoHover(Editor, *ActiveViewport, LocalMouse, ViewportSizePixels);
    } else {
      Gizmo.HoveredHandle = EGizmoHandle::None;
    }

    CameraController.CameraRotateSpeed = Editor.State.GetCameraSensitivity();
    CameraController.CameraMoveSpeed = Editor.State.GetCameraSpeed();

    if (bFocused) {
      FCamera &Camera = ActiveViewport->ViewportCamera;
      CameraController.UpdateMouseInput(Camera);
      if (FInputManager::Get().IsMouseDown(EMouseButton::Right)) {
        CameraController.UpdateKeyInput(Camera, DeltaTime);
      } else {
        if (!Gizmo.IsInteracting()) {
          if (FInputManager::Get().IsKeyJustPressed(VK_OEM_3)) // 백틱 (`)
          {
            if (Gizmo.Mode != EGizmoMode::None &&
                Gizmo.Mode != EGizmoMode::Scale) {
              Gizmo.SetGizmoSpace(static_cast<EGizmoSpace>(
                  (static_cast<uint8>(Gizmo.GetSpace()) + 1) % 2));
            }
          }
          if (FInputManager::Get().IsKeyJustPressed('Q')) {
            Gizmo.Mode = EGizmoMode::None;
          } else if (FInputManager::Get().IsKeyJustPressed('W')) {
            Gizmo.Mode = EGizmoMode::Translate;
          } else if (FInputManager::Get().IsKeyJustPressed('E')) {
            Gizmo.Mode = EGizmoMode::Rotate;
          } else if (FInputManager::Get().IsKeyJustPressed('R')) {
            Gizmo.Mode = EGizmoMode::Scale;
          } else if (FInputManager::Get().IsKeyJustPressed(VK_SPACE)) {
            Gizmo.Mode = static_cast<EGizmoMode>(
                (static_cast<uint8>(Gizmo.Mode) + 1) % 4);
          }
        }
      }
    }
  }

  if (WindowPos.Y < WorkPos.y) {
    WindowPos.Y = WorkPos.y;
    ImVec2 pos = {WindowPos.X, WindowPos.Y};
    ImGui::SetWindowPos(pos);
  }

  ImGui::End();
}

void FImguiEditorViewportWindow::HandlePicking(
    FEditor &Editor, const FEditorViewport &Viewport,
    const FVector2 &LocalMousePixels, const FVector2 &ViewportSizePixels) {
  if (Editor.GetSelectedActor() != nullptr) {
    FGizmo &Gizmo = Editor.GetGizmo();
    if (Gizmo.HoveredHandle != EGizmoHandle::None) {
      Gizmo.BeginInteraction(Editor.SelectedTransform, Gizmo.HoveredHandle,
                             LocalMousePixels, Viewport.ViewportCamera,
                             ViewportSizePixels);
      return;
    }
  }

  TArray<UPrimitiveComponent *> Components = Editor.GetPrimitiveComponents();

  UPrimitiveComponent *HitComponent = nullptr;
  FVector ImpactPoint;

  //		TODO: 뷰포트가 화면 전체가 아니게 되면(예 : 도킹된 하위 창),
  //       FRayCastingManager 가 뷰포트 로컬 마우스 좌표
  //       (FInputManager 좌표 - Viewport.ScreenSpaceTopLeft) 를 인수로 받도록
  //       시그니처를 바꿔야 한다. 지금은 뷰포트 == 클라이언트 영역이라 그대로
  //       사용. ==> DONE
  const bool bHit = FRayCastingManager::RayIntersectsMeshes(
      FRayCastingManager::CreateRayFromScreenPosition(
          Viewport.ViewportCamera, LocalMousePixels, ViewportSizePixels),
      Components, HitComponent, ImpactPoint);




  if (bHit) {
    // 충돌 컴포넌트의 소유 액터 선택
    if (HitComponent && HitComponent->GetActorOwner()) {

        AActor* OwnerActor = HitComponent->GetActorOwner();
        Editor.SelectActor(OwnerActor);
        // 클래스 이름 및 UUID 조회
        const char* ActorClass = OwnerActor->GetClass() ? OwnerActor->GetClass()->GetDisplayName().c_str() : "Unknown";
        const char* CompClass = HitComponent->GetClass() ? HitComponent->GetClass()->GetDisplayName().c_str() : "Unknown";
        // ImGui 콘솔 창에 로그 출력
        UE_LOG("[Picking] Actor: %s (UUID: %u), Component: %s (UUID: %u)",
            ActorClass, OwnerActor->GetUUID(), CompClass, HitComponent->GetUUID());
    
    } else {
      Editor.UnSelectActor();
    }
  } else {
    Editor.UnSelectActor();
  }
}

void FImguiEditorViewportWindow::UpdateGizmoHover(
    FEditor &Editor, const FEditorViewport &Viewport,
    const FVector2 &LocalMousePixels, const FVector2 &ViewportSizePixels) {
  FRay Ray = FRayCastingManager::CreateRayFromScreenPosition(
      Viewport.ViewportCamera, LocalMousePixels, ViewportSizePixels);

  FGizmo &Gizmo = Editor.GetGizmo();
  Gizmo.HoveredHandle =
      Gizmo.HitTest(Editor.SelectedTransform, Ray, Viewport.ViewportCamera);
}
