#pragma once

#include <DirectXMath.h>

class Transform
{
public:
	Transform();

	// setters with overloads
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::XMFLOAT3& pos);

	void SetRotation(float p, float y, float r);
	void SetRotation(const DirectX::XMFLOAT3& rotation);

	void SetScale(float x, float y, float z);
	void SetScale(const DirectX::XMFLOAT3& scale);

	// transformers
	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(const DirectX::XMFLOAT3& offset);

	void MoveRelative(float x, float y, float z);
	void MoveRelative(const DirectX::XMFLOAT3& offset);

	void Rotate(float x, float y, float z);
	void Rotate(const DirectX::XMFLOAT3& rotation);

	void Scale(float x, float y, float z);
	void Scale(const DirectX::XMFLOAT3& scaleFactor);

	// getters
	const DirectX::XMFLOAT3 GetPosition() const;
	const DirectX::XMFLOAT3 GetRotation() const;
	const DirectX::XMFLOAT3 GetScale() const;

	const DirectX::XMFLOAT4X4 GetWorldMatrix();

private:
	// Transform data
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 pitchYawRoll;
	DirectX::XMVECTOR qRotation;
	DirectX::XMFLOAT3 scale;

	// Matrix
	DirectX::XMFLOAT4X4 worldMatrix;

	// track if there are changes
	bool dirty;

	// update quaternion after changing angles
	void UpdateQuaternion();
};

