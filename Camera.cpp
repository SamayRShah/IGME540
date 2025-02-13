#include "Camera.h"
#include "Transform.h"

#include<memory>
#include<DirectXMath.h>

using namespace DirectX;
Camera::Camera(const DirectX::XMFLOAT3& pos, float aspectRatio)
	: fov(XM_PI / 4.0f), nearClip(0.1f), farClip(100.0f), tProjection(perspective), moveSpeed(1.0f), lookSpeed(1.0f)
{
	transform = std::make_shared<Transform>();
	transform->SetPosition(pos);

	UpdateProjectionMatrix(aspectRatio);
	UpdateViewMatrix();
}

void Camera::UpdateProjectionMatrix(float aspectRatio) {
	switch (tProjection) {
	case perspective:
		XMStoreFloat4x4(&mProjection, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearClip, farClip));
		break;
	case orthographic:
		XMStoreFloat4x4(&mProjection, XMMatrixOrthographicLH(aspectRatio * 100.0f, aspectRatio * 100.0f, nearClip, farClip));
		break;
	default:
		return;
	}
}

void Camera::UpdateViewMatrix() {
	return;
}