#pragma once

#include "Mesh.h"
#include "BufferStructs.h"
#include "GameEntity.h"
#include "Camera.h"
#include "Lights.h"
#include "Sky.h"
#include "Window.h"

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

	// game environment vars
	DirectX::XMFLOAT3 bgColor;
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

	// shadow mapping
	int shadowMapResolution = 1024;
	float lightProjectionSize = 20.0f;
	DirectX::XMFLOAT3 slUpDir = DirectX::XMFLOAT3(0, 0, 1);
	float slDistance = -15.0f;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	std::shared_ptr<SimpleVertexShader> shadowVS;
	DirectX::XMFLOAT4X4 lightViewMatrix;
	DirectX::XMFLOAT4X4 lightProjectionMatrix;

	// ==== post processing ====
	Microsoft::WRL::ComPtr<ID3D11SamplerState> ppSampler;
	std::shared_ptr<SimpleVertexShader> ppVS;

	// blur
	int ppBlurRadius = 0;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ppBlurRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ppBlurSRV;
	std::shared_ptr<SimplePixelShader> ppBlurPS;

	// chromatic abberation
	DirectX::XMFLOAT3 ppChromaticOffsets = DirectX::XMFLOAT3(0.009f, 0.006f, -0.006f);
	DirectX::XMFLOAT2 mouseFocusPoint;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ppChromaticRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ppChromaticSRV;
	std::shared_ptr<SimplePixelShader> ppChromaticPS;
	// ===========================


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
	void ResizePostProcessResources();
	void CreateShadowMapResources();
	void ResizeShadowMap();
	void EditShadowMapLight(Light light, float distance);
	std::shared_ptr<Sky> SkyHelper(
		const char* path, std::shared_ptr<Mesh> cube,
		std::shared_ptr<SimpleVertexShader> skyVS, std::shared_ptr<SimplePixelShader> skyPS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);
	std::shared_ptr<Material> MatHelperPBR(
		const char* name, std::shared_ptr<SimpleVertexShader> vs,
		std::shared_ptr<SimplePixelShader> ps,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughness,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metal
		);
	std::shared_ptr<Material> MatHelperDecalPBR(
		const char* name, std::shared_ptr<SimpleVertexShader> vs,
		std::shared_ptr<SimplePixelShader> ps,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> decal,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughness,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metal
	);
	std::shared_ptr<Mesh> MeshHelper(const char* name);
	void EntityHelper(const char* name, std::shared_ptr<Mesh> mesh, 
		std::shared_ptr<Material> mat, DirectX::XMFLOAT3 translate, 
		DirectX::XMFLOAT3 scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f));
	std::shared_ptr<SimpleVertexShader> VSHelper(const std::wstring& filename);
	std::shared_ptr<SimplePixelShader> PSHelper(const std::wstring& filename);

	// === UI Helpers =============
	void UINewFrame(float dt);
	void BuildUI();
	float rtWidth = 256;
	float rtHeight = rtWidth / Window::AspectRatio();
	int selectedLightIndex = -1;
	int selectedEntityIndex = -1;
	int selectedPostProcessIndex = -1;
	std::string selectedCameraName;
	std::string selectedMaterialName;
	std::string openTexturePopupName;
	bool showRenderPasses;

	void UISky();
	void UILights();
	void UIEditShadowLight(Light* light);
	void UIEditLightCommon(Light* light);
	void UIEditShadowMapLight(Light* light);

	void UICameras();
	void UIEditCamera(const std::shared_ptr<Camera>& camera);

	void UIMaterials();
	void UIMaterial(std::shared_ptr<Material>& targetMat);
	void UIEditMaterial(const std::shared_ptr<Material>& mat);
	void UIEditTextureMap(const std::shared_ptr<Material>& mat, const std::string& texName);
	
	void UIEntities();
	void UIMesh(std::shared_ptr<Mesh>& targetMesh);
	void UITransform(Transform& trans);
	void UIEntityDetails(std::shared_ptr<GameEntity> entity);

	void UIShadowMap();
	void UIPostProcessing();
	void UIRenderPasses();
	void UIDetailsBlur();
	void UIDetailsChromaticAberration();
};