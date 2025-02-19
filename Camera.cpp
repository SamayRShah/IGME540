#include "Camera.h"
#include "Transform.h"
#include "Input.h"

#include<memory>
#include<DirectXMath.h>
#include<algorithm>

using namespace DirectX;
Camera::Camera(
	const DirectX::XMFLOAT3& pos,
	float aspectRatio,
	CameraProjectionType projectionType,
	float fov,
	float orthoWidth,
	float nearClip,
	float farClip,
	float moveSpeed,
	float lookSpeed,
	float moveFactor
)
	: aspectRatio(aspectRatio),
	fov(fov),
	orthoWidth(orthoWidth),
	nearClip(nearClip),
	farClip(farClip),
	moveSpeed(moveSpeed),
	lookSpeed(lookSpeed),
	tProjection(projectionType),
	moveFactor(moveFactor)
{
	transform = std::make_shared<Transform>();
	transform->SetPosition(pos);

	UpdateProjectionMatrix(aspectRatio);
	UpdateViewMatrix();
}

const XMFLOAT4X4 Camera::GetProjection() const { return mProjection; }
const XMFLOAT4X4 Camera::GetView() const { return mView; }

void Camera::UpdateProjectionMatrix(float aspectRatio) {
	this->aspectRatio = aspectRatio;

	XMMATRIX mP;

	if (tProjection == Perspective) {
		mP = XMMatrixPerspectiveFovLH(
			fov,
			aspectRatio,
			nearClip,
			farClip
		);
	}
	else {
		mP = XMMatrixOrthographicLH(
			orthoWidth,
			orthoWidth / aspectRatio,
			nearClip,
			farClip
		);
	}

	XMStoreFloat4x4(&mProjection, mP);
}
void Camera::UpdateViewMatrix() {
	XMFLOAT3 pos = transform->GetPosition();
	XMFLOAT3 forward = transform->GetForward();
	XMFLOAT3 up(0, 1, 0);
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&pos),
		XMLoadFloat3(&forward),
		XMLoadFloat3(&up));
	XMStoreFloat4x4(&mView, view);
}
void Camera::Update(float dt) {
	// set move speed
	float speed = dt * moveSpeed;
	
	// running - increase fov and move faster
	if (Input::KeyDown(VK_SHIFT)) {
		speed *= 10.0f;
	}

	// 'crouching' - slow down
	if (Input::KeyDown(VK_CONTROL)) speed *= 0.1f;

	// WASD movement
	if (Input::KeyDown('W')) transform->MoveRelative(0, 0, speed);
	if (Input::KeyDown('A')) transform->MoveRelative(-speed, 0, 0);
	if (Input::KeyDown('S')) transform->MoveRelative(0, 0, -speed);
	if (Input::KeyDown('D')) transform->MoveRelative(speed, 0, 0);
	if (Input::KeyDown(' ')) transform->MoveAbsolute(0, speed, 0);
	if (Input::KeyDown('X')) transform->MoveAbsolute(0, -speed, 0);

	// mouse input
	if (Input::MouseLeftDown()) {

		// get mouse input and rotate
		int cMoveX = Input::GetMouseXDelta();
		int cMoveY = Input::GetMouseYDelta();
		transform->Rotate(cMoveY * lookSpeed, cMoveX * lookSpeed, 0);

		// clamp pitch
		XMFLOAT3 rot = transform->GetRotation();
		if (rot.x > XM_PI || rot.x < -XM_PI) {
			rot.x = std::clamp(rot.x, -XM_PI, XM_PI);
			transform->SetRotation(rot);
		}
	}

	UpdateViewMatrix();
}