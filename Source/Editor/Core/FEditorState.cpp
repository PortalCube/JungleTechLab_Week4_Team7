#include "FEditorState.h"
#include "Editor/Core/FConfigArchive.h"
#include "ThirdParty/mIni/ini.h"

void FEditorState::WriteToFile(FStringView FilePath) const
{
	FConfigArchive Archive;

	// Camera
	Archive.SetFloat("Camera", "Sensitivity", CameraSensitivity);
	Archive.SetFloat("Camera", "Speed", CameraSpeed);
	Archive.SetVector("Camera", "Location", CameraLocation);
	Archive.SetFloat("Camera", "Yaw", CameraYaw);
	Archive.SetFloat("Camera", "Pitch", CameraPitch);

	// Gizmo
	Archive.SetUInt32("Gizmo", "Mode", GizmoMode);
	Archive.SetUInt32("Gizmo", "Space", GizmoSpace);
	Archive.SetUInt32("Gizmo", "SelectedActor", SelectedActor);

	mINI::INIFile File{ FilePath };
	mINI::INIStructure Structure = Archive.GetConfig();

	File.write(Structure, true);
}

void FEditorState::ReadFromFile(FStringView FilePath)
{
	mINI::INIFile File{ FilePath };
	mINI::INIStructure Structure;

	// 파일을 불러오는데 실패하면 기본값 유지
	if (!File.read(Structure))
	{
		UE_LOG("[FEditorState::ReadFromFile] \"%s\" 파일을 불러오는데 실패했습니다.", FilePath);
		return;
	}

	const FConfigArchive Archive{ Structure };

	// Camera

	if (!Archive.IsEmpty("Camera", "Sensitivity"))
	{
		CameraSensitivity = Archive.GetFloat("Camera", "Sensitivity");
	}

	if (!Archive.IsEmpty("Camera", "Speed"))
	{
		CameraSpeed = Archive.GetFloat("Camera", "Speed");
	}

	if (!Archive.IsEmpty("Camera", "Location"))
	{
		CameraLocation = Archive.GetVector("Camera", "Location");
	}

	if (!Archive.IsEmpty("Camera", "Yaw"))
	{
		CameraYaw = Archive.GetFloat("Camera", "Yaw");
	}

	if (!Archive.IsEmpty("Camera", "Pitch"))
	{
		CameraPitch = Archive.GetFloat("Camera", "Pitch");
	}

	// Gizmo

	if (!Archive.IsEmpty("Gizmo", "Mode"))
	{
		GizmoMode = static_cast<uint8>(Archive.GetUInt32("Gizmo", "Mode"));
	}

	if (!Archive.IsEmpty("Gizmo", "Space"))
	{
		GizmoSpace = static_cast<uint8>(Archive.GetUInt32("Gizmo", "Space"));
	}

	if (!Archive.IsEmpty("Gizmo", "SelectedActor"))
	{
		SelectedActor = Archive.GetUInt32("Gizmo", "SelectedActor");
	}

}

void FEditorState::SetCameraSensitivity(float Value)
{
	if (CameraSensitivity == Value) { return; }
	CameraSensitivity = Value;
	WriteToFile();
}

void FEditorState::SetCameraSpeed(float Value)
{
	if (CameraSpeed == Value) { return; }
	CameraSpeed = Value;
	WriteToFile();
}

void FEditorState::SetCameraLocation(const FVector& Value)
{
	if (CameraLocation == Value) { return; }
	CameraLocation = Value;
	WriteToFile();
}

void FEditorState::SetCameraYaw(float Value)
{
	if (CameraYaw == Value) { return; }
	CameraYaw = Value;
	WriteToFile();
}

void FEditorState::SetCameraPitch(float Value)
{
	if (CameraPitch == Value) { return; }
	CameraPitch = Value;
	WriteToFile();
}

void FEditorState::SetGizmoMode(uint8 Value)
{
	if (GizmoMode == Value) { return; }
	GizmoMode = Value;
	WriteToFile();
}

void FEditorState::SetGizmoSpace(uint8 Value)
{
	if (GizmoSpace == Value) { return; }
	GizmoSpace = Value;
	WriteToFile();
}

void FEditorState::SetSelectedActor(uint32 Value)
{
	if (SelectedActor == Value) { return; }
	SelectedActor = Value;
	WriteToFile();
}
