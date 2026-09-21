#include "FImguiPropertyWindow.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Actors/AActor.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include "ThirdParty/Imgui/imgui_impl_dx11.h"
#include "ThirdParty/Imgui/imgui_impl_win32.h"
#include "ThirdParty/Imgui/imgui_stdlib.h"
#include <string>
#include "FImguiDragDrop.h"
#include "Runtime/Rendering/FMaterial.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Asset/FAssetRegistry.h"


namespace
{
	constexpr float SlotSize = 64.0f;
}


void FImguiPropertyWindow::Process(FEditor& Editor)
{
	ImGui::Begin("Jungle Property Window");

	if (AActor* SelectedActor = Editor.GetSelectedActor())
	{
		ShowActorHeader(*SelectedActor);
		ImGui::Separator();

		ShowComponentHierarchy(*SelectedActor);
		ImGui::Separator();

		ShowComponentSections(Editor, *SelectedActor);
	}
	else
	{
		ImGui::TextDisabled("No selection");
	}

	ShowGizmoSettings(Editor);

	ImGui::End();
}

void FImguiPropertyWindow::ShowActorHeader(const AActor& Actor) const
{
	const char* ActorClassName = Actor.GetClass() ? Actor.GetClass()->GetDisplayName().c_str() : "None";
	ImGui::Text("Actor Class: %s", ActorClassName);
	ImGui::Text("Actor UUID: %u", Actor.GetUUID());
}

void FImguiPropertyWindow::ShowComponentHierarchy(const AActor& Actor) const
{
	ImGui::TextDisabled("Components Hierarchy");

	const USceneComponent* RootComp = Actor.GetRootComponent();
	if (RootComp)
	{
		const char* RootName = RootComp->GetClass() ? RootComp->GetClass()->GetDisplayName().c_str() : "RootComponent";
		ImGui::BulletText("[Root] %s (ID: %u)", RootName, RootComp->GetUUID());
	}

	for (const USceneComponent* Comp : Actor.GetAttachedComponents())
	{
		if (!Comp || Comp == RootComp)
		{
			continue;
		}

		const char* SubName = Comp->GetClass() ? Comp->GetClass()->GetDisplayName().c_str() : "SubComponent";
		ImGui::Indent(15.0f);
		ImGui::BulletText("└── [Sub] %s (ID: %u)", SubName, Comp->GetUUID());
		ImGui::Unindent(15.0f);
	}
}

void FImguiPropertyWindow::ShowComponentSections(FEditor& Editor, AActor& Actor)
{
	USceneComponent* RootComp = Actor.GetRootComponent();

	for (USceneComponent* Comp : Actor.GetAttachedComponents())
	{
		if (!Comp)
		{
			continue;
		}

		const bool bIsRoot = (Comp == RootComp);
		const char* CompTypeName = Comp->GetClass() ? Comp->GetClass()->GetDisplayName().c_str() : "Component";

		// ### 뒤쪽이 실제 ID 라서, 앞의 표시 이름이 바뀌어도 접힘 상태가 유지된다.
		std::string SectionTitle = (bIsRoot ? "[Root] " : "[Sub] ") + std::string(CompTypeName)
			+ " (ID: " + std::to_string(Comp->GetUUID()) + ")###CompHeader_" + std::to_string(Comp->GetUUID());

		if (!ImGui::CollapsingHeader(SectionTitle.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			continue;
		}

		// 컴포넌트마다 위젯 ID 를 분리해야 같은 라벨끼리 충돌하지 않는다.
		ImGui::PushID(Comp);
		ShowComponentDetails(Editor, Actor, *Comp, bIsRoot);
		ImGui::PopID();

		ImGui::Spacing();
	}
}

void FImguiPropertyWindow::ShowComponentDetails(FEditor& Editor, AActor& Actor,
	USceneComponent& Comp, bool bIsRoot)
{
	ShowTransform(Editor, Comp, bIsRoot);

	if (Comp.IsA<UTextInstanceComponent>())
	{
		ShowTextSettings(static_cast<UTextInstanceComponent&>(Comp));
	}

	if (Comp.IsA<USpotLightComponent>())
	{
		ShowSpotLightSettings(static_cast<USpotLightComponent&>(Comp));
	}

	else if (Comp.IsA<UStaticMeshComponent>())
	{
		ShowStaticMeshSettings(Actor, static_cast<UStaticMeshComponent&>(Comp), bIsRoot);
	}
}

void FImguiPropertyWindow::ShowTransform(FEditor& Editor, USceneComponent& Comp, bool bIsRoot) const
{
	ImGui::TextDisabled("Transform");

	if (bIsRoot)
	{
		// 루트 컴포넌트 트랜스폼은 에디터 기즈모와 동기화
		ImGui::DragFloat3("Translation", &Editor.SelectedTransform.Location.X, 0.01f);
		if (ImGui::DragFloat3("Rotation (deg)", &Editor.SelectedEulerDegDisplay.X, 0.5f))
		{
			Editor.SelectedTransform.Rotation = FQuaternion::FromEulerXYZDeg(Editor.SelectedEulerDegDisplay);
		}
		ImGui::DragFloat3("Scale", &Editor.SelectedTransform.Scale3D.X, 0.01f);
		return;
	}

	// 서브 컴포넌트 상대 트랜스폼 편집
	FTransform& RelTransform = Comp.GetRelativeTransform();
	ImGui::DragFloat3("Rel Location", &RelTransform.Location.X, 0.01f);

	FVector RelEuler = RelTransform.Rotation.ToEulerXYZDeg();
	if (ImGui::DragFloat3("Rel Rotation (deg)", &RelEuler.X, 0.5f))
	{
		RelTransform.Rotation = FQuaternion::FromEulerXYZDeg(RelEuler);
	}
	ImGui::DragFloat3("Rel Scale", &RelTransform.Scale3D.X, 0.01f);
}

void FImguiPropertyWindow::ShowTextSettings(UTextInstanceComponent& TextComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Text Settings");


	const char* fontItems[] = { "bazziotf", "dnfbitbitv2", "maplestorybold" };
	static int currFontIndex = 0;
	if (ImGui::Combo("Font", &currFontIndex, fontItems, IM_ARRAYSIZE(fontItems)))
	{
		const FName Materials[] = { FName("Material/Instance_Text_Bazzi.json"), FName("Material/Instance_Text_DNF.json"), FName("Material/Instance_Text_Maple.json") };
		const char* selectedFont = fontItems[currFontIndex];
		TextComp.SetMaterial(FAssetRegistry::GetInstance().Get<UMaterial>(Materials[currFontIndex]));
		TextComp.SetFont(FName(selectedFont));
	}

	static char utfBuffer[512]{};
	WideCharToMultiByte(CP_UTF8, 0, TextComp.GetText().c_str(), -1, &utfBuffer[0], sizeof(utfBuffer), NULL, NULL);

	if (ImGui::InputText("Text Content", &utfBuffer[0], sizeof(utfBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		FString Buffer{ &utfBuffer[0] };
		uint32 convertResult = MultiByteToWideChar(CP_UTF8, 0, Buffer.c_str(), Buffer.length(), NULL, 0);
		FWString newText(convertResult, 0);
		MultiByteToWideChar(CP_UTF8, 0, Buffer.c_str(), Buffer.length(), newText.data(), convertResult);
		TextComp.SetText(newText);
	}
}

void FImguiPropertyWindow::ShowSpotLightSettings(USpotLightComponent& LightComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Spot Light Settings");

	FVector LightCol = LightComp.GetLightColor();
	if (ImGui::ColorEdit3("Light Color", &LightCol.X))
	{
		LightComp.SetLightColor(LightCol);
	}

	float LightIntensity = LightComp.GetIntensity();
	if (ImGui::DragFloat("Intensity", &LightIntensity, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetIntensity(LightIntensity);
	}

	float SpotAngle = LightComp.GetSpotAngle();
	if (ImGui::SliderFloat("Spot Angle", &SpotAngle, 1.0f, 89.0f))
	{
		LightComp.SetSpotAngle(SpotAngle);
	}

	float LightRange = LightComp.GetRange();
	if (ImGui::DragFloat("Range", &LightRange, 0.1f, 0.1f, 100.0f))
	{
		LightComp.SetRange(LightRange);
	}
}

void FImguiPropertyWindow::ShowStaticMeshSettings(AActor& Actor, UStaticMeshComponent& MeshComp, bool bIsRoot) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Static Mesh Settings");


	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	if (ImGui::BeginTable(
		"StaticMeshAssetSlots",
		2,
		ImGuiTableFlags_SizingStretchSame
	))
	{
		// 첫 번째 행
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		ShowMaterialSlot(MeshComp);

		ImGui::TableSetColumnIndex(1);
		ShowTextureSlot(MeshComp);

		// 두 번째 행
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		ShowPipelineSlot(MeshComp);

		ImGui::TableSetColumnIndex(1);
		ShowStaticMeshSlot(MeshComp);

		ImGui::EndTable();
	}
}

void FImguiPropertyWindow::ShowMaterialSlot(UStaticMeshComponent& MeshComp) const
{
	UMaterial* Material = MeshComp.GetMaterialInstance()->Material;

	ImGui::Spacing();
	ImGui::TextDisabled("Material");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	ImGui::Button(Material->GetID().ToString().c_str(), ImVec2(FullWidth, SlotSize));

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UMaterial* NewMaterial = Dropped->Ptr->Cast<UMaterial>();

			if (NewMaterial)
			{
				MeshComp.SetMaterial(NewMaterial);
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowPipelineSlot(UStaticMeshComponent& MeshComp) const
{
	UPipeline* Pipeline = MeshComp.GetMaterialInstance()->Pipeline;

	ImGui::Spacing();
	ImGui::TextDisabled("Pipeline");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	ImGui::Button(Pipeline->GetID().ToString().c_str(), ImVec2(FullWidth, SlotSize));

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UPipeline* NewPipeline = Dropped->Ptr->Cast<UPipeline>();

			if (NewPipeline)
			{
				MeshComp.SetPipeline(NewPipeline);
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowTextureSlot(UStaticMeshComponent& MeshComp) const
{
	UMaterial* Material = MeshComp.GetMaterialInstance()->Material;
	FTexture* CurrentTexture = nullptr;
	
	if (Material)
	{
		UTexture* TextureAsset = Material->GetTexture();

		if (TextureAsset)
		{
			CurrentTexture = TextureAsset->Get();
		}
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Texture");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	if (CurrentTexture && CurrentTexture->GetSRV())
	{
		const ImTextureID TexId = reinterpret_cast<ImTextureID>(CurrentTexture->GetSRV());
		ImGui::Image(TexId, ImVec2(FullWidth, SlotSize));
	}
	else
	{
		ImGui::Button("No\nTexture", ImVec2(FullWidth, SlotSize));
	}

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UTexture* Texture = Dropped->Ptr->Cast<UTexture>();

			if (Texture)
			{
				MeshComp.SetTexture(Texture);
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowStaticMeshSlot(UStaticMeshComponent& MeshComp) const
{
	const UStaticMesh* StaticMesh = MeshComp.GetMesh();

	ImGui::Spacing();
	ImGui::TextDisabled("StaticMesh");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	ImGui::Button(StaticMesh->GetID().ToString().c_str(), ImVec2(FullWidth, SlotSize));

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UStaticMesh* NewStaticMesh = Dropped->Ptr->Cast<UStaticMesh>();

			if (NewStaticMesh)
			{
				MeshComp.SetMesh(NewStaticMesh);
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowGizmoSettings(FEditor& Editor) const
{
	static const char* GizmoModes[4] = { "None", "Translation", "Rotation", "Scale" };
	int SelectedItem = static_cast<int>(Editor.GetGizmo().Mode);
	if (ImGui::Combo("Gizmo Mode", &SelectedItem, GizmoModes, 4))
	{
		Editor.GetGizmo().Mode = static_cast<EGizmoMode>(SelectedItem);
	}

	if (SelectedItem == 3) // Scale
	{
		static const char* GizmoSpaces[] = { "Local" };
		SelectedItem = static_cast<int>(Editor.GetGizmo().GetSpace()) - 1;
		if (ImGui::Combo("Gizmo Space", &SelectedItem, GizmoSpaces, 1))
		{
			Editor.GetGizmo().SetGizmoSpace(static_cast<EGizmoSpace>(SelectedItem - 1));
		}
	}
	else if (SelectedItem != 0) // Translation, Rotation
	{
		static const char* GizmoSpaces[] = { "World", "Local" };
		SelectedItem = static_cast<int>(Editor.GetGizmo().GetSpace());
		if (ImGui::Combo("Gizmo Space", &SelectedItem, GizmoSpaces, 2))
		{
			Editor.GetGizmo().SetGizmoSpace(static_cast<EGizmoSpace>(SelectedItem));
		}
	}
}
