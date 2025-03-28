#pragma once
#include "SimpleShader/SimpleShader.h"
#include <DirectXMath.h>
#include <memory>
#include <algorithm>

class Material
{
public:
	// constructor
	Material(const char* name, std::shared_ptr<SimpleVertexShader> vs, 
		std::shared_ptr<SimplePixelShader> ps, DirectX::XMFLOAT3 ct, float r);

	// getters
	const char* GetName() { return name; }
	const DirectX::XMFLOAT3 GetColorTint() const { return colorTint; }
	const float GetRoughness() const { return roughness; }
	const std::shared_ptr<SimplePixelShader> GetPixelShader() { return pixelShader; }
	const std::shared_ptr<SimpleVertexShader> GetVertexShader() { return vertexShader; }
	const DirectX::XMFLOAT2 GetUvScale() const { return uvScale; }
	const DirectX::XMFLOAT2 GetUvOffset() const { return uvOffset; }
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>& GetTextureSRVMap() { return textureSRVs; }
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>>& GetSamplerMap() { return samplers; }

	// setters
	void SetPixelShader(std::shared_ptr<SimplePixelShader> ps) { pixelShader = ps; }
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vs) { vertexShader = vs; }
	void SetColorTint(DirectX::XMFLOAT3 ct) { colorTint = ct; }
	void SetRoughness(float r) { roughness = std::clamp(r, 0.0f, 1.0f); }
	void SetName(const char* n) { name = n; } 
	void SetUvScale(DirectX::XMFLOAT2 s) { uvScale = s; }
	void SetUvOffset(DirectX::XMFLOAT2 off) { uvOffset = off; }

	// texture adders / removers
	void AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	void AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);
	void RemoveTextureSRV(std::string name);
	void RemoveSampler(std::string name);

	// draw helper
	void PrepareMaterial();

private:
	// name for UI
	const char* name;

	// material properties
	DirectX::XMFLOAT3 colorTint;
	float roughness;

	// shaders
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimpleVertexShader> vertexShader;

	// textures
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;
};

