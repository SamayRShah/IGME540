#pragma once

#include "Mesh.h"
#include "Transform.h"

#include <memory>
#include <DirectXMath.h>
#include <wrl/client.h>
class GameEntity
{
public:
	// constructor - requires mesh, overloaded with default name
	GameEntity(const char* name, std::shared_ptr<Mesh> mesh);
	GameEntity(std::shared_ptr<Mesh> mesh);

	// getters
	const std::shared_ptr<Mesh> GetMesh();
	const std::shared_ptr<Transform> GetTransform();
	const char* GetName();

	// color tint
	DirectX::XMFLOAT4 colorTint;
	
	// draw method
	void Draw(Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer);
private:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Transform> transform;
	const char* name;
};

