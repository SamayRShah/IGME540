#include "Game.h"

#include <d3d.h>
#include <format>

using namespace DirectX;
// texture loading helper methods
void Game::LoadPBRTexture(
	const std::wstring& name,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srvA,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srvN,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srvR,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srvM
)
{
	LoadTexture((L"PBR/" + name + L"_" + L"albedo.png"), srvA);
	LoadTexture((L"PBR/" + name + L"_" + L"normals.png"), srvN);
	LoadTexture((L"PBR/" + name + L"_" + L"roughness.png"), srvR);
	LoadTexture((L"PBR/" + name + L"_" + L"metal.png"), srvM);
}

void Game::LoadTexture(
	const std::wstring& path,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv
) {
	const std::wstring& fixedPath = FixPath(L"../../Assets/Textures/" + path);
	DirectX::CreateWICTextureFromFile(
		Graphics::Device.Get(), Graphics::Context.Get(),
		fixedPath.c_str(), nullptr, srv.GetAddressOf());
	lTextureSRVs.push_back(srv);
}

std::shared_ptr<Mesh> Game::MeshHelper(const char* name) {
	std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>(name, FixPath(std::format("../../Assets/Models/{}.obj", name)).c_str());
	umMeshes[newMesh->GetName()] = newMesh;
	return newMesh;
}

void Game::EntityHelper(
	const char* name, std::shared_ptr<Mesh> mesh,
	std::shared_ptr<Material> mat, XMFLOAT3 translate, XMFLOAT3 scale) {
	std::shared_ptr<GameEntity> entity = std::make_shared<GameEntity>(name, mesh, mat);
	entity->GetTransform()->MoveAbsolute(translate);
	entity->GetTransform()->SetScale(scale);
	lEntities.push_back(entity);
}

std::shared_ptr<SimpleVertexShader> Game::VSHelper(const std::wstring& filename) {
	return std::make_shared<SimpleVertexShader>(Graphics::Device, Graphics::Context, FixPath(filename).c_str());
}

std::shared_ptr<SimplePixelShader> Game::PSHelper(const std::wstring& filename) {
	return std::make_shared<SimplePixelShader>(Graphics::Device, Graphics::Context, FixPath(filename).c_str());
}

std::shared_ptr<Sky> Game::SkyHelper(const char* path, std::shared_ptr<Mesh> cube,
	std::shared_ptr<SimpleVertexShader> skyVS, std::shared_ptr<SimplePixelShader> skyPS,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler) {

	std::wstring wPath = std::wstring(path, path + strlen(path));
	std::shared_ptr<Sky> sky = std::make_shared<Sky>(
		FixPath(L"../../Assets/Skies/" + wPath + L"/right.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/left.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/up.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/down.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/front.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/back.png").c_str(),
		cube, skyVS, skyPS, sampler);
	umSkies[path] = sky;
	return sky;
}

std::shared_ptr<Material> Game::MatHelperPBR(
	const char* name,
	std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<SimplePixelShader> ps,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughness,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metal
) {
	XMFLOAT3 ct = XMFLOAT3(1.0f, 1.0f, 1.0f);
	std::shared_ptr<Material> mat = std::make_shared<Material>(name, vs, ps, ct, 0.0f);
	mat->AddSampler("BasicSampler", sampler);
	mat->AddTextureSRV("Albedo", albedo);
	mat->AddTextureSRV("NormalMap", normals);
	mat->AddTextureSRV("RoughnessMap", roughness);
	mat->AddTextureSRV("MetalnessMap", metal);
	umMats[name] = mat;
	return mat;
}

std::shared_ptr<Material> Game::MatHelperDecalPBR(
	const char* name,
	std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<SimplePixelShader> ps,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> decal,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughness,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metal
) {
	XMFLOAT3 ct = XMFLOAT3(1.0f, 1.0f, 1.0f);
	std::shared_ptr<Material> mat = std::make_shared<Material>(name, vs, ps, ct, 0.0f);
	mat->AddSampler("BasicSampler", sampler);
	mat->AddTextureSRV("Albedo", albedo);
	mat->AddTextureSRV("DecalTexture", decal);
	mat->AddTextureSRV("NormalMap", normals);
	mat->AddTextureSRV("RoughnessMap", roughness);
	mat->AddTextureSRV("MetalnessMap", metal);
	umMats[name] = mat;
	return mat;
}
