#pragma once

#include "Runtime/Math/FVector.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/FString.h"

class FConfigArchive;

/// <summary>
/// 에디터에서 필요한 상태들을 담는 클래스입니다.
/// Editor에 값에 대한 멤버 변수가 필요하다면, 여기서 처리 후 저장해주세요.
/// </summary>
class FEditorState
{
public:

	static inline FString DefaultFileName = "editor.ini";

private:
	// Camera
	float CameraSensitivity = 0.5f;
	float CameraSpeed = 10.0f;
	FVector CameraLocation = { 0.0f, 0.0f, 0.0f };
	float CameraYaw = 0.0f;
	float CameraPitch = 0.0f;

	// Gizmo
	uint8 GizmoMode = 0;
	uint8 GizmoSpace = 0;
	uint32 SelectedActor = -1; // Note: uint32이므로 -1은 언더플로우됨

public:
	void WriteToFile(FStringView FilePath = DefaultFileName) const;
	void ReadFromFile(FStringView FilePath = DefaultFileName);

	void SetCameraSensitivity(float Value);
	float GetCameraSensitivity() const { return CameraSensitivity; }

	void SetCameraSpeed(float Value);
	float GetCameraSpeed() const { return CameraSpeed; }

	void SetCameraLocation(const FVector& Value);
	const FVector& GetCameraLocation() const { return CameraLocation; }

	void SetCameraYaw(float Value);
	float GetCameraYaw() const { return CameraYaw; }

	void SetCameraPitch(float Value);
	float GetCameraPitch() const { return CameraPitch; }

	void SetGizmoMode(uint8 Value);
	uint8 GetGizmoMode() const { return GizmoMode; }

	void SetGizmoSpace(uint8 Value);
	uint8 GetGizmoSpace() const { return GizmoSpace; }

	void SetSelectedActor(uint32 Value);
	uint32 GetSelectedActor() const { return SelectedActor; }

};
