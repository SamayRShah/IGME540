#include "Material.h"
using namespace DirectX;

Material::Material(const char* name, std::shared_ptr<SimpleVertexShader> vs,
	std::shared_ptr<SimplePixelShader> ps, DirectX::XMFLOAT4 ct)
	: name(name), pixelShader(ps), vertexShader(vs), colorTint(ct)
{
	uvScale = XMFLOAT2(1, 1);
	uvOffset = XMFLOAT2(0, 0);
}

void Material::AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
	textureSRVs.insert({ name, srv });
}

void Material::AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{
	samplers.insert({ name, sampler });
}

void Material::RemoveTextureSRV(std::string name)
{
	textureSRVs.erase(name);
}

void Material::RemoveSampler(std::string name)
{
	samplers.erase(name);
}

void Material::PrepareMaterial() {

	// Loop and set any other resources
	for (auto& t : textureSRVs) { pixelShader->SetShaderResourceView(t.first.c_str(), t.second.Get()); }
	for (auto& s : samplers) { pixelShader->SetSamplerState(s.first.c_str(), s.second.Get()); }
}
