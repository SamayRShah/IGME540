#pragma once
#include "Mesh.h"
#include "Camera.h"

#include "SimpleShader/SimpleShader.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
class Sky
{
public:
	Sky(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back,
		std::shared_ptr<Mesh> mesh,
		std::shared_ptr<SimpleVertexShader> skyVS,
		std::shared_ptr<SimplePixelShader> skyPS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions);

	void Draw(std::shared_ptr<Camera> cam);

private:
	// mesh
	std::shared_ptr<Mesh> skyMesh;

	// shaders
	std::shared_ptr<SimpleVertexShader> skyVS;
	std::shared_ptr<SimplePixelShader> skyPS;

	// dx11 resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skySRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyDepthState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyRasterState;

	// state initialization helper
	void InitRenderStates();

	// --------------------------------------------------------
	// Author: Chris Cascioli
	// Purpose: Creates a cube map on the GPU from 6 individual textures
	// --------------------------------------------------------
	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);
};

