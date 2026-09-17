# Viewport 리팩토링 작업 문서

작성일: 2026-09-17 / 마감: 2026-09-23 10:00
기준 코드: `FEditorViewport → FEditorViewportClient` 개명 직후 (미커밋 상태)

## 0. 목적·범위

`FEditorViewportClient`가 실제로 ViewportClient 역할을 하도록 흩어진 책임을 끌어모으고,
그 위에 `SWindow/SSplitter` 4분할을 올린다. 명세 요구:

- 세 개의 Viewport 추가, H/V 스플리터로 4등분, 드래그 리사이즈, 스플리터 정보 Editor.ini 저장
- Orthogonal View 동시 렌더링
- 클래스 이름은 UE를 따름 (`FViewport`, `FViewportClient`)
- Perspective 카메라를 씬 JSON(`PerspectiveCamera`)에 저장/복원

원칙 한 줄: **화면마다 달라야 하는 것은 전부 Client 안에, 화면 위치·픽셀·포커스는 Viewport 안에, ImGui는 Viewport에 값을 주입하는 공급자일 뿐.**

## 1. 용어 정의

| 용어 | 한 줄 정의 | 현재 코드 대응 |
|---|---|---|
| `FViewport` | 픽셀 rect + 포커스/호버 + Client 포인터. 에디터/게임/뷰어 공통. ImGui 모름 | 없음 (신설). rect 필드는 지금 `FEditorViewportClient`에 있음 |
| `FViewportClient` | "무엇을 보여줄지" 인터페이스: `ProcessInput / CalcSceneView / Draw` | 없음 (신설) |
| `FEditorViewportClient` | 에디터용 Client: 자유 카메라, 뷰모드, ShowFlags, 기즈모/피킹 입력 해석, 그리드·기즈모 오버레이 | `Source/Editor/EditorViewport/FEditorViewportClient.h` (상태만 있음) |
| `FViewportInput` | Viewport → Client로 넘기는 프레임당 입력 스냅샷 | `FImguiEditorViewportWindow::FViewportInput` (private 중첩) |
| `FSceneView` | 한 프레임·한 뷰포트의 불변 렌더 명세 | `Source/Runtime/Engine/FSceneView.h` |
| `FRenderView` | 씬 패스 실행기 (UE `FSceneRenderer`) | `Source/Runtime/Engine/FRenderView.*` — 현재 에디터 패스까지 겸함 |
| `SWindow` / `SSplitter` | rect 트리. 리프가 `FViewport`를 가짐. 스플리터가 자식 rect를 나눔 | 없음 (신설). 지금은 ImGui 도킹이 대신함 |
| 활성 Client | ControlPanel·저장이 대상으로 삼는 "현재" Client | `FEditor::GetActiveViewport()` = `[0]` 고정 |

## 2. 현재 스냅샷 — 화면별 책임이 지금 어디에 있나

| 책임 | 현재 위치 | 있어야 할 곳 |
|---|---|---|
| 카메라 / 뷰모드 / ShowFlags | `FEditorViewportClient` | Client ✅ |
| rect(UV) / 포커스 / 호버 | `FEditorViewportClient` | **FViewport** |
| rect의 원천 | ImGui 창 (`SyncViewportRect`) | ImGui 창 → 나중에 SSplitter 리프 |
| 입력 수집 (hover/click/로컬좌표) | `FImguiEditorViewportWindow::GatherInput` | Viewport 쪽 (창) ✅ |
| 입력 해석 (피킹/기즈모/카메라/단축키) | `FImguiEditorViewportWindow::Update*` + `CameraController` 멤버 | **Client** |
| FSceneView 조립 | `FEditorApplication::Render` | **Client::CalcSceneView** |
| Aspect 계산·저장 | `SyncViewportRect`, `OnWindowSize` → 카메라에 저장 | Client::CalcSceneView에서 계산 |
| 씬 패스 | `FRenderView::RenderView` 앞부분 | FRenderView ✅ |
| 에디터 패스 (그리드/비주얼라이저/외곽선/기즈모/텍스트) | `FRenderView::RenderView` 뒷부분 | **Client::Draw** |
| 활성 Client 결정 | `FEditor::GetActiveViewport()` `[0]` | FEditor, 클릭 시 갱신 |
| 카메라 저장/복원 | `FEditor::SaveState/LoadState` → editor.ini | Client + 씬 JSON |
| 선택 / 기즈모 모드 / GlobalLight | `FEditor` | FEditor ✅ (전역, 옮기지 않음) |

## 3. 분리 항목

각 항목: **현재 위치 → 옮길 곳 / 왜 이 단계인가 / 완료 판정**

### 3.1 최소조건 — 분할 착수 게이트

이 다섯 개가 끝나면 Client를 N개 만드는 것이 곧 N분할이다. 하나라도 빠지면 그 책임을 N번 복제해야 한다.

#### M1. `FViewport` 신설 + Viewport–Client 소유 구조

- 현재: rect/포커스가 `FEditorViewportClient.h:8-15,22,32-33`에 있음. `FEditor.h:82` `TArray<FEditorViewportClient>` 값 배열.
- 옮길 곳: `Source/Runtime/Engine/FViewport.h` (신설). `FEditor`가 Viewport/Client를 `TUniquePtr`로 소유.
- 왜 이 단계: SSplitter 리프가 가리킬 대상이 필요하고, ImGui 창이 rect를 "주입"할 대상이 필요하다. 값 배열은 `push_back` 시 주소가 바뀌어 Client 포인터가 깨진다.
- 완료 판정: `FEditorViewportClient`에 rect/포커스 필드가 없다. `FViewport.h`가 `imgui.h`와 `Editor/*`를 include하지 않는다.

#### M2. 입력 해석 → `FEditorViewportClient::ProcessInput`

- 현재: `FImguiEditorViewportWindow.cpp:121-282` (`UpdateSelection`, `HandlePicking`, `UpdateGizmo`, `UpdateGizmoHover`, `UpdateCamera`, `UpdateShortcuts`) + `FImguiEditorViewportWindow.h:72` `CameraController`.
- 옮길 곳: `FEditorViewportClient`. 창에는 `BeginWindow/EndWindow/SyncViewportRect/GatherInput/ClampWindowToWorkArea`만 남는다.
- 왜 이 단계: 각 화면이 자기 입력에만 반응해야 한다. `CameraController`는 `Velocity` 상태를 가지므로 화면별로 하나씩 있어야 한다.
- 완료 판정: 창의 `Process`가 `Client->ProcessInput(Viewport, Input, dt)` 한 줄로 해석을 위임한다. 창이 `FGizmo`, `FRayCastingManager`를 include하지 않는다.

#### M3. FSceneView 조립 → `FEditorViewportClient::CalcSceneView`

- 현재: `FEditorApplication.cpp:69-77`. Aspect는 `FImguiEditorViewportWindow.cpp:79`와 `FEditorApplication.cpp:100-110`에서 카메라에 저장.
- 옮길 곳: `FEditorViewportClient::CalcSceneView(const FViewport&)`. Aspect는 여기서 `Viewport.GetRect()`로 계산해 넘기고 저장하지 않는다.
- 왜 이 단계: 화면별 카메라·모드로 각자 FSceneView를 만들어야 한다. Aspect를 저장하면 rect 바뀔 때마다 갱신 지점이 두 곳(SyncRect, OnWindowSize)이 된다.
- 완료 판정: `FEditorApplication::Render`에 `FSceneView{ ... }` 리터럴이 없다. `OnWindowSize`에서 카메라를 건드리지 않는다.

#### M4. 에디터 패스 → `FEditorViewportClient::Draw`

- 현재: `FRenderView.cpp:80-112` (Grid, Visualizer, `FlushLinePass`, `RenderPostProcessPass`, `RenderOverlayPass`) + `FEditorApplication.cpp:80-92` (`FEditorRenderContext` 조립).
- 옮길 곳: `FEditorViewportClient::Draw(const FViewport&, FRenderView&)`. `FRenderView`에는 `RenderScene`(BeginView + Collect + BasePass)와 개별 헬퍼(`FlushLinePass`, `RenderOutline`, PDI 계열)만 남는다.
- 왜 이 단계: 그리드·기즈모는 각 화면의 카메라로 각자 그려야 한다. 지금은 `RenderView()` 한 함수에 씬 패스와 에디터 패스가 순서로 박혀 있어 화면별 분기가 불가능하다.
- 완료 판정: `FRenderView.cpp`에 `#include "Editor/..."`가 없다. `FEditorRenderContext`가 사라지거나 Client 내부용이 된다.

#### M5. 활성 Client 추적

- 현재: `FEditor.cpp:135-140` `GetActiveViewport()`가 `[0]` 고정.
- 옮길 곳: `FEditor::ActiveViewportIndex` + `SetActiveViewport(int32)`. 창(나중에 SSplitter 리프)이 **클릭(좌/우) 시** 호출.
- 왜 이 단계: ControlPanel/저장/단축키가 "어느 화면"인지 알아야 한다. 호버가 아니라 클릭 기준인 이유: 호버 기준이면 마우스가 지나갈 때마다 ControlPanel 표시가 바뀐다 (UE `GCurrentLevelEditingViewportClient`도 클릭/포커스 시 갱신).
- 완료 판정: 두 번째 뷰포트를 클릭한 뒤 ControlPanel의 ViewMode 콤보가 그 뷰포트 값을 보여준다.

### 3.2 권장 — 분할과 병행 가능

최소조건 뒤에 하면 되고, 분할 트랙과 파일이 겹치지 않는 것부터.

#### R1. ControlPanel → 활성 Client 메서드 호출

- 현재: `FImguiControlPanelWindow.cpp:158-195` (ViewMode/ShowFlags), `:197-277` (Ortho 체크, FOV/위치/Yaw/Pitch, Reset Camera). Client 필드를 직접 쓰고 `Editor.State`에도 직접 기록.
- 조정: `Client->SetViewMode()`, `ToggleShowFlag()`, `SetViewportType()`, `ResetCamera()` 호출로. Reset의 `State.SetCamera*` 직접 기록은 R2와 중복이므로 제거.
- 왜 권장: 동작은 M5만으로도 맞다. 필드 직접 접근이 남으면 R5(ViewportType) 도입 때 Ortho 체크박스와 충돌한다.
- 완료 판정: ControlPanel이 `ViewportCamera`, `ViewMode`, `ShowFlags` 필드에 직접 대입하지 않는다.

#### R2. 카메라 저장: editor.ini → Client / 씬 JSON

- 현재: `FEditor.cpp:73-102` `SaveState/LoadState`가 `[0]` Client 카메라를 `FEditorState`(editor.ini)에 저장. `FEditor::Process`가 매 프레임 `SaveState()` 호출.
- 조정: 명세대로 Perspective 카메라는 씬 JSON `PerspectiveCamera`로 (`USceneManager::SaveScene/LoadScene`에 훅). Ortho 카메라는 저장 불필요(고정 방향, 줌/패닝만). `FEditorState`에는 Sensitivity/Speed(전역 설정)만 남긴다.
- 왜 권장: 명세 요구사항이지만 분할과 독립. 단 4분할 후 `[0]`이 Persp라는 보장이 없어지므로 M5 뒤에.
- 완료 판정: 씬 저장 파일에 `PerspectiveCamera` 블록이 있고 로드 시 Persp Client에 복원된다. `FEditorState`에 `CameraLocation/Yaw/Pitch/FOV`가 없다.

#### R3. `RenderOutline` 전체화면 → rect 단위

- 현재: `FRenderer.cpp:856` `RSSetViewports(1, &Viewport)`로 전체 크기 복구 후 풀스크린 삼각형.
- 조정: 그 한 줄 제거. `OutlinePostProcessPS.hlsl`이 `SV_Position` 절대 픽셀로 `Load`하므로 서브 rect로 클리핑만 되면 셰이더 수정 없이 뷰포트별 외곽선이 된다.
- 왜 권장: 1개 뷰포트에서는 문제가 안 보인다. 2개 이상이면 뒤에 그린 뷰포트의 후처리가 앞 뷰포트를 덮어쓴다 → **분할 트랙에서 2개 뷰포트가 보이는 순간 필요**.
- 완료 판정: 좌/우 2뷰포트에서 각자 외곽선이 그려지고 서로 덮지 않는다.

#### R4. 기즈모 드래그 소유 Client 가드

- 현재: `FGizmo.h:46-83` `BeginInteraction/UpdateInteraction` 상태(`InteractionOriginViewport`, `InteractionAxisViewport`)가 전역 기즈모 안에 있음. 어느 Client든 `bLeftDown`이면 `UpdateInteraction`을 부른다.
- 조정: `FEditorViewportClient`에 `bool bOwnsGizmoDrag`. `BeginInteraction`을 부른 Client만 `Update/End`를 보낸다.
- 왜 권장: 드래그 중 마우스가 다른 뷰포트로 넘어가면 그쪽 Client가 자기 카메라 기준 좌표로 `UpdateInteraction`을 호출해 오브젝트가 튄다. 1개 뷰포트에서는 재현 불가.
- 완료 판정: 뷰포트 A에서 드래그 시작 후 B 위로 마우스를 옮겨도 A 카메라 기준으로 계속 움직인다.

#### R5. ViewportType + Ortho 입력 해석

- 현재: Ortho는 `Camera.Projection.ProjectionType` 토글뿐. 카메라 방향 고정·패닝·줌 없음.
- 조정: `enum class ELevelViewportType { Perspective, OrthoTop, OrthoFront, OrthoSide }`를 Client에. Ortho면 Yaw/Pitch 고정, 우클릭 드래그 = 패닝, 휠 = `Projection.Height` 줌. `ProcessInput` 내부 분기.
- 왜 권장: 명세 "Orthogonal View 동시 렌더링"의 실체. 분할 트랙에서 4개 인스턴스 만들 때 타입만 다르게 주면 된다.
- 완료 판정: Top/Front/Side 뷰포트에서 우클릭 드래그로 화면이 평행이동하고 회전하지 않는다.

#### R6. DELETE 키 포커스 판정

- 현재: `FEditor.cpp:54` `FInputManager::Get().IsKeyDown(VK_DELETE)` — 어느 패널이 포커스든 발동.
- 조정: `ProcessInput`에서 `Input.bFocused`일 때 Client가 `Editor->DeleteSelectedActor()` 호출. 명령 자체는 `FEditor`에 남긴다.
- 왜 권장: 지금도 버그(Property 창에서 텍스트 편집 중 Delete 누르면 액터 삭제)지만 분할과 무관.
- 완료 판정: ImGui 텍스트 필드에 포커스가 있을 때 Delete가 액터를 지우지 않는다.

### 3.3 원칙상 분리 — 이번 주 필수 아님

UE 경계와 정확히 맞추기 위한 것. 4분할 동작에는 영향 없음.

| # | 항목 | 현재 | 원칙 |
|---|---|---|---|
| P1 | `FViewportClient` 순수가상 인터페이스 확정 | M1에서 빈 베이스로 시작 | `ProcessInput/CalcSceneView/Draw` 순수가상. 두 번째 Client(OBJ Viewer)가 생길 때 확정 |
| P2 | `FViewport`가 렌더 프레임 경계 담당 | `FRenderView::BeginView`가 RT 바인딩 + `SetViewportUV` | `FViewport::BeginRenderFrame(FRenderer&)`로 이동. `FSceneView`에서 rect 제거. 새 OS 창/오프스크린 RT는 여기에 붙는다 |
| P3 | `FViewportInput` 단일 소스화 | 클릭/호버는 ImGui, 키·우클릭·마우스 델타는 `FInputManager` 직접 | Client가 `FInputManager`를 직접 읽지 않고 `FViewportInput`만 본다. 키 상태·우클릭·델타를 `FViewportInput`에 추가 |
| P4 | 선택 강조 색상을 Runtime에서 제거 | `FRenderView::CollectScenePrimitives:56-64` `bSelected` 틴트 | 에디터 관심사. Client::Draw 또는 Collect 콜백으로 |
| P5 | `CreateRayFromScreenPosition` → `FSceneView` 메서드 | `FRayCastingManager` 자유 함수 | UE `FSceneView::DeprojectScreenToWorld`. Client가 만든 View로 바로 역투영 |
| P6 | `FVisualizerRegistry` 소유 이동 | `FEditorApplication` 멤버 | `FEditor` (에디터 전역). M4에서 Client::Draw가 접근하려면 어차피 `FEditor` 경유가 자연스러움 — **M4 하면서 같이 옮겨도 됨** |

## 4. 시그니처 초안 (M1~M5)

각 결정의 이유를 옆에 둔다. 구현 중 바뀌면 이유도 같이 갱신할 것.

### 4.1 `Source/Runtime/Math/FRect.h` (신설)

```cpp
struct FRect {
    FVector2 TopLeft{};   // 픽셀, 창 클라이언트 영역 기준
    FVector2 Size{};      // 픽셀
    [[nodiscard]] bool Contains(const FVector2& P) const;
    [[nodiscard]] float Aspect() const { return Size.Y > 0.f ? Size.X / Size.Y : 1.f; }
};
```

- **픽셀이지 UV가 아닌 이유**: 명세 `SWindow{FRect Rect; IsHover(FPoint)}`가 픽셀이고, UE `FViewport::GetSizeXY()`도 픽셀이며, 스플리터 드래그·호버 판정·Aspect·레이 생성(`CreateRayFromScreenPosition(…, SizePixels)`)이 전부 픽셀을 원한다. 지금 UV를 쓰는 곳은 `FRenderer::SetViewportUV` 하나뿐이고, 그것도 내부에서 픽셀로 되돌린다. UV의 유일한 장점(창 리사이즈 시 갱신 불필요)은 SSplitter가 루트 rect에서 리프를 재계산하므로 사라진다.
- **`FVector2` 두 개인 이유**: 기존 수학 타입 재사용. `Min/Max` 대신 `TopLeft/Size`인 것은 `SetViewportRect`와 `SizePixels` 소비처가 그 형태를 원해서.

### 4.2 `Source/Runtime/Engine/FViewportInput.h` (창 private 구조체를 꺼냄)

```cpp
struct FViewportInput {
    FVector2 LocalMouse{};     // 뷰포트 좌상단 기준 픽셀
    FVector2 SizePixels{};     // == Viewport.GetRect().Size — 편의 복사
    bool bHovered = false;
    bool bFocused = false;
    bool bPickRequested = false;   // 좌클릭 "발생" (ImGui IsItemClicked)
    bool bLeftDown = false;
    bool bLeftReleased = false;
};
```

- **필드를 그대로 두는 이유**: M2는 "이동"이지 "재설계"가 아니다. 지금 `Update*` 4개가 정확히 이 필드들만 쓰므로 그대로 옮기면 동작 동일이 보장된다. 키·우클릭·델타는 지금처럼 Client가 `FInputManager`를 직접 읽는다 — 이것이 P3에서 정리할 타협.
- **Runtime에 두는 이유**: `FViewportClient::ProcessInput` 시그니처에 들어가므로 Runtime에서 보여야 한다. ImGui 의존 없음(값만 있음).

### 4.3 `Source/Runtime/Engine/FViewportClient.h` (신설)

```cpp
class FViewport;
class FRenderView;
struct FSceneView;
struct FViewportInput;

class FViewportClient {
public:
    virtual ~FViewportClient() = default;
    virtual void ProcessInput(FViewport& Viewport, const FViewportInput& Input, float DeltaTime) = 0;
    virtual FSceneView CalcSceneView(const FViewport& Viewport) const = 0;
    virtual void Draw(const FViewport& Viewport, FRenderView& RenderView) = 0;
};
```

- **지금 순수가상 3개를 두는 이유**: `FViewport`(Runtime)가 Client를 가리키려면 Runtime에 타입이 있어야 한다. `FEditorViewportClient`(Editor)를 직접 가리키면 Runtime→Editor 의존이 생긴다. 이 세 개가 "Viewport가 Client에게 시키는 일"의 전부다.
- **`ProcessInput`이 `FViewport&`를 받는 이유**: Client가 `Viewport.GetRect()`(레이 생성, 기즈모 `WorldToViewport`)와 포커스를 조회한다. UE `InputKey(FViewport*, …)`와 같은 형태.
- **`CalcSceneView`가 `const`이고 값 반환인 이유**: 한 프레임짜리 명세라 복사가 싸다. `FSceneView::Camera`가 `const FCamera&`라 Client의 카메라를 참조하지만 Client가 프레임 내내 살아 있으므로 안전. 렌더 중 카메라가 바뀌면 안 된다는 규약이 여기서 나온다(입력 처리는 Update, 렌더는 Render — 이미 분리돼 있음).
- **`Draw`가 `FRenderView&`를 받는 이유**: 그리드/기즈모/PDI 호출이 전부 `FRenderView` 메서드다. `FRenderer&`를 직접 주면 Client가 D3D 세부(`ClearDepth`, `SetViewportUV`)를 알게 된다 — 그건 P2에서 `FViewport::BeginRenderFrame`이 가져갈 것.

### 4.4 `Source/Runtime/Engine/FViewport.h` (신설)

```cpp
class FViewport final {
public:
    void SetRect(const FRect& InRect) { Rect = InRect; }
    [[nodiscard]] const FRect& GetRect() const { return Rect; }

    void UpdateFocusedAndHovered(bool bInFocused, bool bInHovered);
    [[nodiscard]] bool IsFocused() const { return bFocused; }
    [[nodiscard]] bool IsHovered() const { return bHovered; }

    void SetClient(FViewportClient* InClient) { Client = InClient; }
    [[nodiscard]] FViewportClient* GetClient() const { return Client; }

private:
    FRect Rect{};
    bool bFocused = false;
    bool bHovered = false;
    FViewportClient* Client = nullptr;   // 비소유
};
```

- **`imgui.h`, `Editor/*` include 금지**: 이것이 "범용"의 정의. OBJ Viewer Configuration은 `SetRect({0,0,W,H})` 한 줄로 이 클래스를 그대로 쓴다.
- **Client를 비소유 포인터로 두는 이유**: UE도 `FViewport::ViewportClient`는 raw 포인터, 소유는 바깥(`SLevelViewport`). 여기서는 `FEditor`가 둘 다 소유한다(4.6). 소유를 FViewport에 넣으면 Client 타입을 알아야 해서 Runtime→Editor 의존이 다시 생긴다.
- **RT 관련 멤버가 없는 이유**: 지금은 백버퍼 서브 rect 방식이라 RT는 `FRenderer` 전역 1세트. P2에서 `BeginRenderFrame(FRenderer&)`를 추가할 때 여기에 붙는다. 미리 넣으면 빈 함수만 남는다.
- **`final`인 이유**: 새 OS 창/오프스크린 RT가 필요해지면 상속이 아니라 P2의 `BeginRenderFrame` 내부 분기 또는 RT 핸들 멤버로 처리할 계획. 상속 열어두면 `FEditorViewport` 같은 잘못된 파생이 생기기 쉽다.

### 4.5 `Source/Editor/EditorViewport/FEditorViewportClient.h` (기존 수정)

```cpp
class FEditorViewportClient final : public FViewportClient {
public:
    // 상태 — 화면마다 다른 것
    FCamera ViewportCamera;
    EViewModeIndex ViewMode = EViewModeIndex::VMI_Lit;
    uint64 ShowFlags = SF_Primitives | SF_BillboardText;
    // R5: ELevelViewportType ViewportType = ELevelViewportType::Perspective;

    void Initialize(FEditor* InEditor) { Editor = InEditor; }

    void ProcessInput(FViewport& Viewport, const FViewportInput& Input, float DeltaTime) override;
    FSceneView CalcSceneView(const FViewport& Viewport) const override;
    void Draw(const FViewport& Viewport, FRenderView& RenderView) override;

    // 기존 유지
    bool HasShowFlag(EEngineShowFlags) const;  void ToggleShowFlag(EEngineShowFlags);

private:
    // M2에서 창에서 이동. 시그니처의 (FEditor&, const FEditorViewportClient&) 인자는 멤버가 되므로 제거
    void UpdateSelection(FViewport&, const FViewportInput&);
    void UpdateGizmo(FViewport&, const FViewportInput&);
    void UpdateCamera(FViewport&, const FViewportInput&, float DeltaTime);
    void UpdateShortcuts();
    void HandlePicking(const FVector2& LocalMouse, const FVector2& SizePixels);
    void UpdateGizmoHover(const FVector2& LocalMouse, const FVector2& SizePixels);

    FEditor* Editor = nullptr;            // 비소유. 선택/기즈모/씬/그리드 접근
    FCameraInputController CameraController;  // 화면별 Velocity 상태
    // R4: bool bOwnsGizmoDrag = false;
};
```

- **`FEditor*`를 갖는 이유**: 옮겨오는 `Update*` 코드가 `Editor.GetGizmo()`, `Editor.SelectActor()`, `Editor.GetPrimitiveComponents()`, `Editor.State.GetCameraSpeed()`를 쓴다. 전역(선택·기즈모)은 `FEditor`에 남기기로 했으므로(7장) Client가 `FEditor`를 알아야 한다. UE `FLevelEditorViewportClient`가 `GEditor`를 쓰는 것과 같다. 순환 include는 `FEditor.h`가 `FEditorViewportClient.h`를 include하므로 여기서는 전방선언만.
- **`CameraController`가 Client 멤버인 이유**: `Velocity`가 있어서 화면마다 독립이어야 한다. 창 멤버로 두면 4개 뷰포트가 가속 상태를 공유한다.
- **카메라·모드가 public 필드로 남는 이유**: M단계에서는 이동만. R1에서 ControlPanel을 메서드 호출로 바꿀 때 private + Setter로 전환한다. 지금 바꾸면 ControlPanel까지 한 커밋에 묶여 팀 충돌 범위가 커진다.

### 4.6 `Source/Editor/Core/FEditor.h` (기존 수정)

```cpp
class FEditor {
public:
    // 생성/파괴
    int32 AddViewport(TUniquePtr<FEditorViewportClient> Client);   // Viewport 생성 + Client 연결, 인덱스 반환
    void  RemoveViewport(int32 Index);

    // 조회
    [[nodiscard]] int32 NumViewports() const;
    [[nodiscard]] FViewport&             GetViewport(int32 Index);
    [[nodiscard]] FEditorViewportClient& GetViewportClient(int32 Index);

    // 활성
    void SetActiveViewport(int32 Index);
    [[nodiscard]] FViewport*             GetActiveViewport();
    [[nodiscard]] FEditorViewportClient* GetActiveViewportClient();

private:
    TArray<TUniquePtr<FViewport>>             Viewports;        // 인덱스 = SSplitter 리프 순서
    TArray<TUniquePtr<FEditorViewportClient>> ViewportClients;  // Viewports[i] <-> ViewportClients[i]
    int32 ActiveViewportIndex = 0;
};
```

- **`FEditor`가 둘 다 소유하는 이유**: UE에서 소유자는 `SLevelViewport`(Slate 위젯)다. 여기서 대응물은 SSplitter 리프(`SWindow`)인데 아직 없다. 리프가 생겨도 리프는 `FViewport*`만 가리키고 소유는 `FEditor`에 두는 편이 OBJ Viewer(리프 없음)와 맞다.
- **`TUniquePtr` 두 배열인 이유**: 주소 안정성. `FViewport::Client`, ImGui 창/SSplitter 리프의 `FViewport*`, 활성 포인터가 전부 `push_back`에 안전해야 한다. 두 배열을 인덱스로 짝짓는 것은 "Viewport 1 : Client 1"이 명세 범위에서 고정이기 때문이다. 짝 구조체 하나로 묶어도 되지만, 그러면 `FViewport`가 Runtime이고 Client가 Editor라 구조체가 Editor에 놓이고 `GetViewport(i)`가 한 단계 더 들어간다 — 이점이 없어 두 배열로.
- **활성을 인덱스로 두는 이유**: 포인터로 두면 `RemoveViewport` 때 dangling 관리가 필요. 인덱스는 `Clamp`만 하면 된다.
- **`GetActiveViewport()`가 `FViewport*`를 반환하는 이유**: 기존 호출처(ControlPanel 등)는 카메라를 원하므로 `GetActiveViewportClient()`로 바꾼다. 이름을 나눠 두면 "rect가 필요한가, 카메라가 필요한가"가 호출부에서 드러난다.

### 4.7 `FImguiEditorViewportWindow::Process` (M2~M5 이후 형태)

```cpp
void FImguiEditorViewportWindow::Process(FEditor& Editor, float DeltaTime)
{
    FViewport* Viewport = Editor.GetActiveViewport();     // 분할 전: 0번 하나
    if (!Viewport) return;

    BeginWindow();
    SyncViewportRect(*Viewport);                           // ImGui 창 rect → Viewport.SetRect (픽셀)
    const FViewportInput Input = GatherInput(Viewport->GetRect());
    Viewport->UpdateFocusedAndHovered(Input.bFocused, Input.bHovered);

    if (Input.bPickRequested || FInputManager::Get().IsMouseDown(EMouseButton::Right))
        Editor.SetActiveViewport(/*this index*/);          // M5: 클릭 기준

    if (FViewportClient* Client = Viewport->GetClient())
        Client->ProcessInput(*Viewport, Input, DeltaTime);

    ClampWindowToWorkArea();
    EndWindow();
}
```

- 창이 하는 일이 "ImGui에서 값을 읽어 Viewport에 넣고, Client를 부른다"로 줄어든다. 분할 트랙에서 이 창은 SSplitter 루트 컨테이너가 되고, `SyncViewportRect`/`GatherInput`이 리프별로 돈다.

### 4.8 `FEditorApplication::Render` (M3~M4 이후 형태)

```cpp
void FEditorApplication::Render()
{
    for (int32 i = 0; i < Editor.NumViewports(); ++i) {
        FViewport& Viewport = Editor.GetViewport(i);
        FViewportClient* Client = Viewport.GetClient();
        if (!Client) continue;

        const FSceneView View = Client->CalcSceneView(Viewport);
        RenderView->RenderScene(View, *SceneManager->CurrentScene, Editor.GetSelectedActor());
        Client->Draw(Viewport, *RenderView);
    }
    ImguiManager.RenderUI();
}
```

- **`RenderScene`이 `SelectedActor`를 받는 이유**: `CollectScenePrimitives`의 선택 틴트(P4) 때문. P4 전까지는 인자로 넘기는 것이 최소 변경.
- **`FSceneView`에 rect가 남는 이유**: `RenderScene → BeginView → SetViewportRect`가 rect를 원한다. P2에서 `Viewport.BeginRenderFrame(Renderer)`가 그 역할을 가져가면 `FSceneView`에서 뺀다. M단계에서는 `CalcSceneView`가 `Viewport.GetRect()`를 복사해 넣는다. `FRenderer`에는 `SetViewportRect(const FRect&)`를 추가하고 기존 `SetViewportUV`는 그 래퍼로 남긴다 — M단계에서 `Runtime/Rendering`을 건드리는 유일한 지점.

## 5. 착수 게이트 자가 테스트

M1~M5 완료 후, **SSplitter 없이** 아래를 수행한다.

1. `Initialize_Runtime`에서 Client 2개 생성, `Viewports[0].SetRect(좌반)`, `Viewports[1].SetRect(우반)` 하드코딩.
2. ImGui 창은 임시로 두 rect를 순회하며 `GatherInput`(각 rect에 `InvisibleButton` 하나씩).
3. 확인:
   - [ ] 왼쪽 우클릭 회전 → 왼쪽만 회전
   - [ ] 오른쪽에서 `W/E/R` → 기즈모 모드는 전역이므로 **양쪽 다** 바뀜 (이게 맞음)
   - [ ] 오른쪽 클릭으로 액터 선택 → 양쪽에 같은 선택 표시, 기즈모는 각자 카메라로 그려짐
   - [ ] 오른쪽 클릭 후 ControlPanel ViewMode 변경 → 오른쪽만 Wireframe
   - [ ] 오른쪽에서 외곽선이 왼쪽을 덮지 않음 (R3 필요 — 덮으면 R3를 먼저)
   - [ ] 창 리사이즈 후 두 rect가 갱신되고 Aspect가 맞음

통과하면 분할 트랙 시작. 실패 항목은 "하나뿐인 카메라/뷰포트"를 암묵적으로 가정한 코드가 남은 것.

## 6. 병행 계획 — 두 트랙

최소조건 충족 후 동시 진행. 겹치는 파일은 굵게.

| 트랙 | 항목 | 건드리는 파일 |
|---|---|---|
| **[분할]** | `FRect`, `SWindow`, `SSplitter`, `SSplitterH/V` 신설 | `Source/Editor/Slate/`(신설 폴더) |
| | ImGui 창 → 루트 컨테이너, 리프별 rect 주입·입력 수집 | **`FImguiEditorViewportWindow`** |
| | Client 4개 생성(Persp + Top/Front/Side), 활성 갱신 | **`FEditorApplication`**, **`FEditor`** |
| | 스플리터 비율 Editor.ini 저장/복원 | `FEditorState`, `FConfigArchive` |
| | `OnWindowSize` → 루트 rect 재설정 | `FEditorApplication`, `main.cpp` |
| **[분리]** | R1 ControlPanel → Client 메서드 | `FImguiControlPanelWindow`, **`FEditorViewportClient`** |
| | R2 카메라 저장 → 씬 JSON | **`FEditor`**, `USceneManager`, `FEditorState` |
| | R3 RenderOutline rect | `FRenderer` |
| | R4 기즈모 드래그 가드 | **`FEditorViewportClient`** |
| | R5 ViewportType/Ortho 입력 | **`FEditorViewportClient`** |
| | R6 DELETE 포커스 | **`FEditor`**, **`FEditorViewportClient`** |

- R3는 분할 트랙에서 2뷰포트가 보이는 즉시 필요 → 분할 트랙 첫 커밋 직전에 넣는다.
- R5는 분할 트랙 "Client 4개 생성"과 같은 커밋이 자연스럽다 (타입 인자를 주면서 생성).
- `FEditorViewportClient`는 두 트랙이 모두 건드린다 → 분리 트랙의 R1/R4/R5/R6는 각각 작은 커밋으로 자주 병합.

## 7. 옮기지 말 것 — 에디터 전역

| 항목 | 위치 | UE 대응 | 이유 |
|---|---|---|---|
| 선택 (`SelectActor/UnSelectActor`, `SelectedTransform`) | `FEditor` | `USelection`/`GEditor` | 4개 뷰포트가 같은 선택을 본다 |
| 기즈모 모드/공간 (`FGizmo::Mode`, `ModeSpace`) | `FEditor::Gizmo` | `FEditorModeTools::WidgetMode` | W/E/R은 전역. 드래그 **상태**만 R4로 Client가 가드 |
| `GlobalLight` | `FEditor` | 씬 | 뷰포트별 라이트는 없다 |
| 카메라 감도/속도 | `FEditorState` | `ULevelEditorViewportSettings` | 전역 설정 |
| `FGrid`, `FVisualizerRegistry` | `FEditor` / `FEditorApplication`(P6) | 전역 리소스 | Client::Draw가 참조만 |
| 액터 삭제/스폰 명령 | `FEditor` | 레벨 에디터 액션 | 포커스 **판정**만 Client(R6) |
