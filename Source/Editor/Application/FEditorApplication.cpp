#include "FEditorApplication.h"

#include "Runtime/CoreUObject/FGarbageCollector.h"
#include "Runtime/CoreUObject/FReferenceCollector.h"
#include "Runtime/CoreUObject/UAnimatedBillboardComp.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"
#include "Runtime/CoreUObject/UCubeComp.h"
#include "Runtime/CoreUObject/UCylinderComp.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/CoreUObject/UTextComponent.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Rendering/FMesh.h"
#include <Windows.h>


#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/AInstancingActor.h"
#include "Runtime/Actors/TestTextActor.h"
#include "Runtime/CoreUObject/UPlaneComp.h"
#include "Runtime/CoreUObject/USphereComp.h"
#include "Runtime/CoreUObject/UTextComponent.h"

void FEditorApplication::Initialize_ImguiWin32DX11(
    HWND &Window, ID3D11Device *Device, ID3D11DeviceContext *Context) {
  ImguiManager.Initialize_ImplWin32DX11(Window, Device, Context);
}

void FEditorApplication::Initialize_Runtime(USceneManager *SceneManager,
                                            FRenderView *RenderView) {
  this->RenderView = RenderView;
  this->SceneManager = SceneManager;
  this->CurrentScene = SceneManager->CurrentScene;

  Editor.Initialize(SceneManager);

  FEditorViewport Viewport;
  Viewport.ViewportCamera.Position = FVector{-3.0f, 3.0f, 2.0f};
  Viewport.ViewportCamera.Pitch = -25.0f;
  Viewport.ViewportCamera.Yaw = -45.0f;
  Viewport.TopLeftUV = {0.0f, 0.0f};
  Viewport.LengthUV = {0.7f, 0.7f};
  Editor.AddViewport(Viewport);
  // Editor.LoadScene("");
}

/// <summary>
/// return value: if scene is pre-existing, returns true
/// if scene was not existing, returns false
/// </summary>
/// <param name="path"></param>
/// <returns></returns>
bool FEditorApplication::CheckSceneExistsAndInitializeIfNotExists(
    const FString &path) {
  if (Editor.CheckSceneExists())
    return true;
  else {
    if (path == "")
      Editor.NewScene();
    else
      Editor.LoadScene(path);
    return false;
  }
}

void FEditorApplication::Update(float DeltaTime) {
  BeginFrame();
  Tick(DeltaTime);
}

void FEditorApplication::BeginFrame() { ImguiManager.NewFrame(); }

void FEditorApplication::Tick(float DeltaTime) {
  ToolBar.Process(Editor, ConsoleWindow, ControlPanelWindow, PropertyWindow);
  EditorViewportWindow.Process(Editor, DeltaTime);
  WorldOutliner.Process(Editor);
  ControlPanelWindow.Process(Editor);
  PropertyWindow.Process(Editor);
  ConsoleWindow.Process(Editor);
  ContentsDrawer.Process(Editor);
  Editor.Process();
}

void FEditorApplication::Render() {
  const TArray<FEditorViewport> &EditorViewports = Editor.GetViewports();

  for (auto &EditorViewport : EditorViewports) {
    if (RenderView) {
      RenderView->GetRenderer().SetRenderMode(EditorViewport.ViewMode);
      RenderView->GetRenderer().UpdateLightConstants(Editor.GlobalLight, EditorViewport.ViewMode); // globallgiht udpate
    }

    RenderView->RenderGrid(EditorViewport.ViewportCamera,
                           EditorViewport.TopLeftUV, EditorViewport.LengthUV,
                           Editor.GetGrid()); // 그리드 그리기

    for (auto& PrimitiveComponent : SceneManager->CurrentScene->GetRenderComponents())
    {
        if (!PrimitiveComponent) continue;

        if (!EditorViewport.HasShowFlag(PrimitiveComponent->GetShowFlag()))
        {
            continue;
        }


        bool bSelected = true;

        if (!PrimitiveComponent) { bSelected = false; }
        else if (!PrimitiveComponent->GetActorOwner()) { bSelected = false; }
        else if (PrimitiveComponent->GetActorOwner() != Editor.GetSelectedActor()) { bSelected = false; }

        RenderView->Render
        (
            EditorViewport.ViewportCamera,
            EditorViewport.TopLeftUV,
            EditorViewport.LengthUV,
            PrimitiveComponent,
            bSelected
        );
    }



    if (Editor.ObjectSelected())
    {
        // AABB 그리기
        USceneComponent* RootComp = Editor.GetSelectedActor()->GetRootComponent();
        UPrimitiveComponent* PrimComp = RootComp->Cast<UPrimitiveComponent>();
        if (PrimComp && PrimComp->GetMesh())
        {
            if (PrimComp->IsA<USpotLightComponent>())
            {
                auto Mesh = PrimComp->GetMesh();
                const FMatrix ModelMatrix = PrimComp->GetModelMatrix();
                const auto& Positions = Mesh->GetPositions();
                const auto& Indices = Mesh->GetIndices();
                const FVector4 WireColor{ 1.0f, 1.0f, 0.0f, 1.0f }; // 노란색 선
                // 메쉬의 삼각형 인덱스를 순회하며 모서리 선 그리기
                for (size_t i = 0; i + 2 < Indices.size(); i += 3)
                {
                    FVector A = ModelMatrix.TransformPointRow(Positions[Indices[i]]);
                    FVector B = ModelMatrix.TransformPointRow(Positions[Indices[i + 1]]);
                    FVector C = ModelMatrix.TransformPointRow(Positions[Indices[i + 2]]);
                    RenderView->RenderLine(A, B, WireColor);
                    RenderView->RenderLine(B, C, WireColor);
                    RenderView->RenderLine(C, A, WireColor);
                }
            }
            else
            {
                // AABB 그리기
                const FMesh& Mesh = *PrimComp->GetMesh();
                const FMatrix ModelMatrix = PrimComp->GetModelMatrix();
                FAxisAlignedBoundingBox AABB{ Mesh, ModelMatrix };
                RenderView->RenderBoxMinMax(AABB.Min, AABB.Max, FVector4{ 1.0f, 1.0f, 1.0f, 1.0f });
            }
        }
    }

    RenderView->GetRenderer().FlushLineBatch(
        EditorViewport.ViewportCamera
            .CreateViewProjectionMatrix()); // line batch 일괄 flush

    if (Editor.ObjectSelected()) {
      // 기즈모 그리기
      RenderView->RenderGizmo(
          Editor.SelectedTransform, EditorViewport.ViewportCamera,
          EditorViewport.TopLeftUV, EditorViewport.LengthUV, Editor.GetGizmo());
    }

    // 선택 객체 하이라이트 렌더
  }
  ImguiManager.RenderUI();
}

void FEditorApplication::OnWindowSize(UINT Width, UINT Height) {
  // 뷰포트 종횡비 갱신
  for (auto &Viewport : Editor.GetViewports()) {
    const FVector2 SizePixels =
        Viewport.LengthUV *
        FVector2{static_cast<float>(Width), static_cast<float>(Height)};

    auto &Camera = Viewport.ViewportCamera;
    Camera.Projection.Aspect = SizePixels.X / SizePixels.Y;
  }
}

void FEditorApplication::CollectGarbage() {
  FGarbageCollector::Get().CollectGarbage();
}
