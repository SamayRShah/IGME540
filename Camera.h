#pragma once

#include "Transform.h"

#include <DirectXMath.h>
#include <memory>

enum CameraProjectionType {
	orthographic,
	perspective
};
class Camera
{
public:
	Camera(const DirectX::XMFLOAT3& pos, float aspectRatio);

	const DirectX::XMFLOAT4X4 GetView();
	const DirectX::XMFLOAT4X4 GetProjection();
private:
	float fov;
	float nearClip;
	float farClip;
	float moveSpeed;
	float lookSpeed;

	CameraProjectionType tProjection;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProjection;

	std::shared_ptr<Transform> transform;

	void UpdateProjectionMatrix(float aspectRatio);
	void UpdateViewMatrix();
};

