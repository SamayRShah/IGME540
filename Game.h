#pragma once

#include "Mesh.h"
#include "BufferStructs.h"
#include "GameEntity.h"
#include "Camera.h"
#include "Lights.h"

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

	// vectors to hold GameEntities
	std::vector<std::shared_ptr<GameEntity>> lEntities;
	// unordered maps for cams, mats, meshes
	std::unordered_map<std::string, std::shared_ptr<Camera>> umCameras;
	std::unordered_map<std::string, std::shared_ptr<Material>> umMats;
	std::unordered_map<std::string, std::shared_ptr<Mesh>> umMeshes;

	// helper methods
	std::shared_ptr<Mesh> MeshHelper(const char* name);
	void EntityHelper(const char* name, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat, DirectX::XMFLOAT3 translate);
	std::shared_ptr<SimpleVertexShader> VSHelper(const std::wstring& filename);
	std::shared_ptr<SimplePixelShader> PSHelper(const std::wstring& filename);
};