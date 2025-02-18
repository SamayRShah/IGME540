#pragma once

#include "Mesh.h"
#include "Transform.h"
#include "Camera.h"

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
	const std::shared_ptr<Mesh> GetMesh() const { return mesh; }
	const std::shared_ptr<Transform> GetTransform() const { return transform; }
	const char* GetName() const { return name; }
	const DirectX::XMFLOAT4 GetColorTint() const { return colorTint; }
	
	// setters
	void SetName(const char* name) { this->name = name; }
	void SetColorTint(DirectX::XMFLOAT4 tint) { colorTint = tint; }

	// draw method
	void Draw(Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer, std::shared_ptr<Camera> cam);
private:
	// mesh and transform pointers
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Transform> transform;

	// name and color tint member vars
	const char* name;
	DirectX::XMFLOAT4 colorTint;
};

