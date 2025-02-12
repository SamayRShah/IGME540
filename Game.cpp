#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"

// ImGui includes
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// mesh class include
#include "Mesh.h"

#include <DirectXMath.h>
#include <memory>
#include <format>

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Called once per program, after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
void Game::Initialize()
{
	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(Window::Handle());
	ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());
	// Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateGeometry();

	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		Graphics::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.
		Graphics::Context->IASetInputLayout(inputLayout.Get());

		// Set the active vertex and pixel shaders
		//  - Once you start applying different shaders to different objects,
		//    these calls will need to happen multiple times per frame
		Graphics::Context->VSSetShader(vertexShader.Get(), 0, 0);
		Graphics::Context->PSSetShader(pixelShader.Get(), 0, 0);

		// create constant buffer
		UINT bufferSize = sizeof(VertexShaderExternalData);
		bufferSize = (bufferSize + 15) / 16 * 16;

		// create a constant buffer
		D3D11_BUFFER_DESC cbDesc{};
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.ByteWidth = bufferSize;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		Graphics::Device->CreateBuffer(&cbDesc, 0, constantBuffer.GetAddressOf());

		// Bind the buffer
		Graphics::Context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
	}
}


// --------------------------------------------------------
// Clean up memory or objects created by this class
// 
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game()
{
	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}


// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* vertexShaderBlob;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts
	{
		// Read our compiled shader code files into blobs
		// - Essentially just "open the file and plop its contents here"
		// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
		// - Note the "L" before the string - this tells the compiler the string uses wide characters
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create the actual Direct3D shaders on the GPU
		Graphics::Device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
			pixelShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

		Graphics::Device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
			vertexShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer
	}

	// Create an input layout 
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a color, which is 4 more float values
		inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
		inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Create the input layout, verifying our description against actual shader code
		Graphics::Device->CreateInputLayout(
			inputElements,							// An array of descriptions
			2,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}
}


// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMFLOAT4 white = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4 gray = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

	// Set up the vertices of the triangle we would like to draw
	// - We're going to copy this array, exactly as it exists in CPU memory
	//    over to a Direct3D-controlled data structure on the GPU (the vertex buffer)
	// - Note: Since we don't have a camera or really any concept of
	//    a "3d world" yet, we're simply describing positions within the
	//    bounds of how the rasterizer sees our screen: [-1 to +1] on X and Y
	// - This means (0,0) is at the very center of the screen.
	// - These are known as "Normalized Device Coordinates" or "Homogeneous 
	//    Screen Coords", which are ways to describe a position without
	//    knowing the exact size (in pixels) of the image/window/etc.  
	// - Long story short: Resizing the window also resizes the triangle,
	//    since we're describing the triangle in terms of the window itself
	Vertex vertices[] =
	{
		{ XMFLOAT3(+0.0f, +0.5f, +0.0f), red },
		{ XMFLOAT3(+0.5f, -0.5f, +0.0f), blue },
		{ XMFLOAT3(-0.5f, -0.5f, +0.0f), green },
	};

	// Set up indices, which tell us which vertices to use and in which order
	// - This is redundant for just 3 vertices, but will be more useful later
	// - Indices are technically not required if the vertices are in the buffer 
	//    in the correct order and each one will be used exactly once
	// - But just to see how it's done...
	UINT indices[] = { 0, 1, 2 };

	Vertex vertices2[] =
	{
		{ XMFLOAT3(0.0f, 0.0f, 0.0f), red },
		{ XMFLOAT3(0.5f, 0.0f, 0.0f), blue },
		{ XMFLOAT3(0.5f, -0.3f, 0.0f), blue },
		{ XMFLOAT3(0.0f, -0.3f, 0.0f), red },
	};
	UINT indices2[] = { 0, 1, 2, 2, 3, 0 };

	Vertex vertices3[] =
	{
		{ XMFLOAT3(-0.05f, 0.35f, 0.0f), gray },
		{ XMFLOAT3(0.35f, 0.15f, 0.0f), gray },
		{ XMFLOAT3(0.0f, 0.15f, 0.0f), white },
		{ XMFLOAT3(0.10f, 0.05f, 0.0f), white },
		{ XMFLOAT3(0.10f, -0.05f, 0.0f), white },
		{ XMFLOAT3(0.0f, -0.15f, 0.0f), white },
		{ XMFLOAT3(0.35f, -0.15f, 0.0f), gray },
		{ XMFLOAT3(-0.05f, -0.35f, 0.0f), gray },
		{ XMFLOAT3(-0.10f, -0.05f, 0.0f), white },
		{ XMFLOAT3(-0.10f, 0.05f, 0.0f), white }
	};
	UINT indices3[] = { 0, 1, 2, 3, 4, 2, 3, 4, 5, 6, 7, 5, 5, 8, 9, 2, 8, 9 };

	// add meshes to lists
	std::shared_ptr<Mesh> m1 = std::make_shared<Mesh>("Triangle", vertices, 3, indices, 3);
	std::shared_ptr<Mesh> m2 = std::make_shared<Mesh>("Rectangle", vertices2, 4, indices2, 6);
	std::shared_ptr<Mesh> m3 = std::make_shared<Mesh>("space ship", vertices3, 10, indices3, 18);

	// transform and color data
	std::shared_ptr<GameEntity> e1 = std::make_shared<GameEntity>("triangle 1", m1);
	e1->GetTransform()->Scale(0.5f, 0.5f, 1.0f);
	lEntities.push_back(e1);

	std::shared_ptr<GameEntity> e2 = std::make_shared<GameEntity>("triangle 2", m1);
	e2->GetTransform()->SetPosition(-0.5f, 0.5f, 0.0f);
	lEntities.push_back(e2);

	std::shared_ptr<GameEntity> e3 = std::make_shared<GameEntity>("Rectangle", m2);
	e3->GetTransform()->SetPosition(0.25f, 0.8f, 0.0f);
	e3->colorTint = XMFLOAT4(1, 0, 1, 1);
	lEntities.push_back(e3);

	std::shared_ptr<GameEntity> e4 = std::make_shared<GameEntity>("Space Ship", m3);
	e4->GetTransform()->SetPosition(0.55f, 0.0f, 0.0f);
	e4->colorTint = XMFLOAT4(1, 0, 1, 1);
	lEntities.push_back(e4);

	std::shared_ptr<GameEntity> e5 = std::make_shared<GameEntity>("Space Ship 2", m3);
	e5->GetTransform()->SetPosition(0.55f, -0.7f, 0.0f);
	e5->GetTransform()->SetScale(0.5f, 0.5f, 1.0f);
	e5->colorTint = XMFLOAT4(0, 0, 1, 1);
	lEntities.push_back(e5);
}


// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
}


// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// setup new frame for ImGui and build the ui
	UINewFrame(deltaTime);
	BuildUI();

	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();
}


// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erase what's on screen) and depth buffer
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(),	bgColor);
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// draw meshes
	for (size_t i = 0; i < lEntities.size(); i++) {
		lEntities[i]->Draw(constantBuffer);
	}

	// prepare ImGui buffers
	ImGui::Render(); // Turns this frame’s UI into renderable triangles
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present at the end of the frame
		bool vsync = Graphics::VsyncState();
		Graphics::SwapChain->Present(
			vsync ? 1 : 0,
			vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Re-bind back buffer and depth buffer after presenting
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());
	}
}

// --------------------------------------------------------
// Prepare new frame for the UI
// --------------------------------------------------------
void Game::UINewFrame(float deltaTime) {

	// Feed fresh data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)Window::Width();
	io.DisplaySize.y = (float)Window::Height();

	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Determine new input capture
	Input::SetKeyboardCapture(io.WantCaptureKeyboard);
	Input::SetMouseCapture(io.WantCaptureMouse);
}

// --------------------------------------------------------
// Build UI for the current frame
// --------------------------------------------------------
void Game::BuildUI() {
	if (showUIDemoWindow) {
		ImGui::ShowDemoWindow();
	}

	// Open/create custom window
	ImGui::Begin("Inspector");
	{
		// make tree node and make it open by default
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("App Details"))
		{
			// show frame rate and window size
			ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
			ImGui::Text("Window Client Size: %dx%d", Window::Width(), Window::Height());

			// 'close' tree node
			ImGui::TreePop();
		}

		// editor for background color
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("World Properties"))
		{
			ImGui::ColorEdit4("Bg Color", bgColor);
			ImGui::TreePop();
		}

		// change ui color style and hide / show demo window
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("UI Options"))
		{
			ImGui::Text("UI Style:");
			{
				if (ImGui::Button("Classic"))
					ImGui::StyleColorsClassic();
				ImGui::SameLine();
				if (ImGui::Button("Light"))
					ImGui::StyleColorsLight();
				ImGui::SameLine();
				if (ImGui::Button("Dark"))
					ImGui::StyleColorsDark();
			}

			if (ImGui::Button(showUIDemoWindow ? "Hide ImGui Demo Window" : "Show ImGui Demo Window"))
				showUIDemoWindow = !showUIDemoWindow;

			ImGui::TreePop();
		}

		// mesh ui
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Entities")) {
			for (size_t i = 0; i < lEntities.size(); i++) {

				// get address for entity, mesh, and transform
				GameEntity& targetEntity = *lEntities[i].get();
				Mesh& targetMesh = *targetEntity.GetMesh();
				Transform& targetTransform = *targetEntity.GetTransform();

				ImGui::PushID(static_cast<UINT>(i));
				if (ImGui::TreeNode(std::format("Entity: {}", targetEntity.GetName()).c_str())) {
					if (ImGui::TreeNode(std::format("Mesh: {}", targetMesh.GetName()).c_str())) {
						ImGui::Text("triangles: %d", targetMesh.GetTriCount());
						ImGui::Text("vertices: %d", targetMesh.GetVertexCount());
						ImGui::Text("indeces: %d", targetMesh.GetIndexCount());
						ImGui::TreePop();
					}
					ImGui::TreePop();

					// color tint
					ImGui::ColorEdit4("color tint", reinterpret_cast<float*>(&targetEntity.colorTint));

					// position
					XMFLOAT3 pos = targetTransform.GetPosition();
					ImGui::DragFloat3("Position", reinterpret_cast<float*>(&pos), 0.1f, -FLT_MAX, FLT_MAX, "%.3f");
					targetTransform.SetPosition(pos.x, pos.y, pos.z);

					// rotation
					XMFLOAT3 rotation = targetTransform.GetRotation();
					ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&rotation), 0.1f, -FLT_MAX, FLT_MAX, "%.3f");
					targetTransform.SetRotation(rotation.x, rotation.y, rotation.z);

					// scale
					XMFLOAT3 scale = targetTransform.GetScale();
					ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&scale), 0.1f, -FLT_MAX, FLT_MAX, "%.3f");
					targetTransform.SetScale(scale.x, scale.y, scale.z);
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}
	// close window
	ImGui::End();

}


