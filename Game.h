#pragma once

#include "Mesh.h"
#include "BufferStructs.h"
#include "GameEntity.h"
#include "Camera.h"
#include "Lights.h"
#include "Sky.h"

// texture loading helpers
#include "Graphics.h"
#include "PathHelpers.h"
#include <WICTextureLoader.h>

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
class Game
{
public:
	// Basic OOP setup
	Game() = default;
	~Game();
	Game(const Game&) = delete; // Remove copy constructor
	Game& operator=(const Game&) = delete; // Remove copy-assignment operator

	// Primary functions
	void Initialize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void OnResize();

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void CreateGeometry();

	// Helper methods for ImGui
	void UINewFrame(float deltaTime);
	void BuildUI();
	bool showUIDemoWindow; // track state of demo window

	// game environment vars
	DirectX::XMFLOAT3 bgColor;
	DirectX::XMFLOAT3 ambientColor;
	std::vector<Light> lights;

	// active camera
	std::string activeCamName;
	std::shared_ptr<Camera> activeCamera;

	// active sky box
	std::string activeSkyName;
	std::shared_ptr<Sky> activeSky;

	// vectors to hold GameEntities
	std::vector<std::shared_ptr<GameEntity>> lEntities;

	// unordered maps for cams, mats, meshes, skies, and textures
	std::unordered_map<std::string, std::shared_ptr<Camera>> umCameras;
	std::unordered_map<std::string, std::shared_ptr<Material>> umMats;
	std::unordered_map<std::string, std::shared_ptr<Mesh>> umMeshes;
	std::unordered_map<std::string, std::shared_ptr<Sky>> umSkies;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> lTextureSRVs;

	// texture loading helper methods
	void LoadPBRTexture(
		const std::wstring& name,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srvA,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srvN,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srvR,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srvM
	);
	void LoadTexture(
		const std::wstring& path,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv
	);

	// helper methods
	std::shared_ptr<Sky> SkyHelper(
		const char* path, std::shared_ptr<Mesh> cube, 
		std::shared_ptr<SimpleVertexShader> skyVS, std::shared_ptr<SimplePixelShader> skyPS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler, 
		DirectX::XMFLOAT3 ambientColor = DirectX::XMFLOAT3(0.1f, 0.15f, 0.18f));
	std::shared_ptr<Material> MatHelperPhong(
		const char* name, std::shared_ptr<SimpleVertexShader> vs, 
		std::shared_ptr<SimplePixelShader> ps, 
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo, 
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals);
	std::shared_ptr<Material> MatHelperPBR(
		const char* name, std::shared_ptr<SimpleVertexShader> vs,
		std::shared_ptr<SimplePixelShader> ps,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughness,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metal
		);
	std::shared_ptr<Mesh> MeshHelper(const char* name);
	void EntityHelper(const char* name, std::shared_ptr<Mesh> mesh, 
		std::shared_ptr<Material> mat, DirectX::XMFLOAT3 translate);
	std::shared_ptr<SimpleVertexShader> VSHelper(const std::wstring& filename);
	std::shared_ptr<SimplePixelShader> PSHelper(const std::wstring& filename);
};