#pragma once

struct FCamera;

class FCameraInputController
{
public:
	void UpdateKeyInput(FCamera& Camera, float DeltaTime) const;
	void UpdateMouseInput(FCamera& Camera) const;

	float CameraMoveSpeed = 10.0f;
	float CameraRotateSpeed = 0.5f;
};
