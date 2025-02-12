#include "GameEntity.h"
#include "Transform.h"
#include "Graphics.h"
#include "BufferStructs.h"

#include <memory>

// constructors
GameEntity::GameEntity(const char* name, std::shared_ptr<Mesh> mesh) :
	mesh(mesh),
	name(name),
	colorTint(1,1,1,1)
{
	transform = std::make_shared<Transform>();
}
GameEntity::GameEntity(std::shared_ptr<Mesh> mesh) : GameEntity("Entity", std::move(mesh)) {}

// getters
const std::shared_ptr<Mesh> GameEntity::GetMesh() { return mesh; }
const std::shared_ptr<Transform> GameEntity::GetTransform() { return transform; }
const char* GameEntity::GetName() { return name; }

void GameEntity::Draw(Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer)
{
	// get buffer location in GPU memory
	D3D11_MAPPED_SUBRESOURCE mapped;
	Graphics::Context->Map(
		constantBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped
	);

	// store transform and color data
	VertexShaderExternalData mData = {
		colorTint,
		transform.get()->GetWorldMatrix()
	};

	// copy data to gpu
	memcpy(mapped.pData, &mData, sizeof(VertexShaderExternalData));

	// unmap data when done - allow gpu to access that memory
	Graphics::Context->Unmap(constantBuffer.Get(), 0);

	// draw mesh
	mesh->Draw();
}