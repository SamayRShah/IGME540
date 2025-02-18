#include "GameEntity.h"
#include "Transform.h"
#include "Graphics.h"
#include "BufferStructs.h"
#include "Camera.h"

#include <memory>
#include <DirectXMath.h>

using namespace DirectX;
// constructors
GameEntity::GameEntity(const char* name, std::shared_ptr<Mesh> mesh) :
	mesh(mesh),
	name(name),
	colorTint(1,1,1,1)
{
	transform = std::make_shared<Transform>();
}
GameEntity::GameEntity(std::shared_ptr<Mesh> mesh) : GameEntity("Entity", std::move(mesh)) {}

void GameEntity::Draw(Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer, std::shared_ptr<Camera> cam)
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
		transform.get()->GetWorldMatrix(),
		cam->GetView(),
		cam->GetProjection()
	};

	// copy data to gpu
	memcpy(mapped.pData, &mData, sizeof(VertexShaderExternalData));

	// unmap data when done - allow gpu to access that memory
	Graphics::Context->Unmap(constantBuffer.Get(), 0);

	// draw mesh
	mesh->Draw();
}