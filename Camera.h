#pragma once

#include "Transform.h"

#include <DirectXMath.h>
#include <memory>

enum CameraProjectionType {
	Perspective,
	Orthographic
};
class Camera
{
public:
	// constructor
	Camera(
		const DirectX::XMFLOAT3& pos,
		float aspectRatio,
		CameraProjectionType projectionType = CameraProjectionType::Perspective,
		float fov = DirectX::XM_PIDIV4,
		float orthoWidth = 10.0f,
		float nearClip = 0.01f,
		float farClip = 100.0f,
		float moveSpeed = 1.0f,
		float lookSpeed = 0.002f,
		float moveFactor = 5.0f
	);

	// getters
	CameraProjectionType GetProjectionType() const { return tProjection; }
	float GetAspectRatio() const { return aspectRatio; }
	float GetFOV() const { return fov; }
	float GetOrthoWidth() const { return orthoWidth; }
	float GetNearClip() const { return nearClip; }
	float GetFarClip() const { return farClip; }
	float GetMoveSpeed() const { return moveSpeed; }
	float GetLookSpeed() const { return lookSpeed; }
	float GetMoveFactor() const { return moveFactor; }
	const DirectX::XMFLOAT4X4 GetView() const;
	const DirectX::XMFLOAT4X4 GetProjection() const;
	const std::shared_ptr<Transform> GetTransform() const { return transform; }


	// setters
	void SetProjectionType(CameraProjectionType type) { tProjection = type; UpdateProjectionMatrix(aspectRatio); }
	void SetAspectRatio(float ar) { aspectRatio = ar; UpdateProjectionMatrix(ar); }
	void SetFOV(float f) { fov = f >= 0.01f ? f : 0.01f; UpdateProjectionMatrix(aspectRatio); }
	void SetOrthoWidth(float w) { orthoWidth = w; UpdateProjectionMatrix(aspectRatio); }
	void SetNearClip(float nc) { nearClip = nc; UpdateProjectionMatrix(aspectRatio); }
	void SetFarClip(float fc) { farClip = fc; UpdateProjectionMatrix(aspectRatio); }
	void SetMoveSpeed(float ms) { moveSpeed = ms; }
	void SetLookSpeed(float ls) { lookSpeed = ls; }
	void SetMoveFactor(float mf) { moveFactor = mf; }

	// update functions
	void UpdateProjectionMatrix(float aspectRatio);
	void Update(float dt);

private:

	// modifiable member variables
	float aspectRatio;
	float fov;
	float orthoWidth;
	float nearClip;
	float farClip;
	float moveSpeed;
	float lookSpeed;
	float moveFactor;
	CameraProjectionType tProjection = Perspective;

	// matrices and transform
	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProjection;
	std::shared_ptr<Transform> transform;

	void UpdateViewMatrix();
};

