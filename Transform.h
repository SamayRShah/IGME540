#pragma once

#include <DirectXMath.h>

class Transform
{
public:
	Transform();

	// setters
	void SetPosition(float x, float y, float z);
	void SetRotation(float p, float y, float r);
	void SetScale(float x, float y, float z);

	// transformers
	void MoveAbsolute(float x, float y, float z);
	void MoveRelative(float x, float y, float z);
	void Rotate(float x, float y, float z);
	void Scale(float x, float y, float z);

	// getters
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetRotation();
	DirectX::XMFLOAT3 GetScale();

	DirectX::XMFLOAT4X4 GetWorldMatrix();
private:

	// Transform data
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 pitchYawRoll;
	DirectX::XMFLOAT3 scale;

	// Matrix
	DirectX::XMFLOAT4X4 worldMatrix;

	// track if there are changes
	bool dirty;
};

