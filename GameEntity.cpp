#include "GameEntity.h"
#include "Transform.h"
#include "Graphics.h"
#include "BufferStructs.h"
#include "Camera.h"

#include <memory>
#include <DirectXMath.h>

using namespace DirectX;
// constructors
GameEntity::GameEntity(const char* name, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat)
	: mesh(mesh), name(name), material(mat)
{
	transform = std::make_shared<Transform>();
}
GameEntity::GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat) 
	: GameEntity("Entity", std::move(mesh), std::move(mat)) {}

void GameEntity::Draw(std::shared_ptr<Camera> cam, float dt, float tt)
{
	std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();
	std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();

	// activate shaders
	vs->SetShader();
	ps->SetShader();

	// set vertex shader data
	vs->SetMatrix4x4("mWorld", transform->GetWorldMatrix());
	vs->SetMatrix4x4("mProj", cam->GetProjection());
	vs->SetMatrix4x4("mView", cam->GetView());
	vs->SetFloat("dt", dt);
	vs->SetFloat("tt", tt);
	vs->CopyAllBufferData();

	// set pixel shader data
	ps->SetFloat4("colorTint", material->GetColorTint());
	ps->SetFloat("dt", dt);
	ps->SetFloat("tt", tt);
	ps->SetFloat2("uvScale", material->GetUvScale());
	ps->SetFloat2("uvOffset", material->GetUvOffset());
	ps->CopyAllBufferData();

	// prepare material
	material->PrepareMaterial();

	// draw mesh
	mesh->Draw();
}