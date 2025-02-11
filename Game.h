#pragma once

#include "Mesh.h"
#include "BufferStructs.h"
#include "GameEntity.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <DirectXMath.h>
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
	void LoadShaders();
	void CreateGeometry();

	// Helper methods for ImGui
	void UINewFrame(float deltaTime);
	void BuildUI();
	bool showUIDemoWindow; // track state of demo window

	// game background color
	float bgColor[4] = { 0.4f, 0.6f, 0.75f, 0.0f };

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	// vectors to hold meshes and their color / transform data and Vertex copy data
	std::vector<std::shared_ptr<Mesh>> lMeshes;
	std::vector<std::shared_ptr<GameEntity>> lEntities;

	// constant buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
};

