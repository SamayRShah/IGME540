#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"

// ImGui & simple shaders includes
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "SimpleShader/SimpleShader.h"

// assignment class includes
#include "Mesh.h"
#include "Material.h"

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

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
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
	}

	// setup cameras
	{
		umCameras = {
			{"Main Camera", std::make_shared<Camera>(DirectX::XMFLOAT3(12, 8, -25), Window::AspectRatio())},
			{"Top Ortho", std::make_shared<Camera>(DirectX::XMFLOAT3(10, 10, 0), Window::AspectRatio(), Orthographic, XM_PIDIV4, 30.0f)},
			{"Side Ortho", std::make_shared<Camera>(DirectX::XMFLOAT3(30, 0, 0), Window::AspectRatio(), Orthographic, XM_PIDIV4, 30.0f)},
			{"Top Perspective", std::make_shared<Camera>(DirectX::XMFLOAT3(10, 10, -3.5), Window::AspectRatio(), Perspective, XMConvertToRadians(130))},
			{"Side Perspective", std::make_shared<Camera>(DirectX::XMFLOAT3(30, 0, -3.5), Window::AspectRatio(), Perspective, XMConvertToRadians(130))}
		};

		umCameras["Top Ortho"]->GetTransform()->SetRotation(XM_PIDIV2, 0, XM_PI);
		umCameras["Side Ortho"]->GetTransform()->SetRotation(0, -XM_PIDIV2, 0);
		umCameras["Top Perspective"]->GetTransform()->SetRotation(XM_PIDIV4, 0, 0);
		umCameras["Side Perspective"]->GetTransform()->SetRotation(0, -XMConvertToRadians(68), 0);
		umCameras["Main Camera"]->GetTransform()->SetRotation(XMConvertToRadians(14), -XMConvertToRadians(-3), 0);


		activeCamName = "Main Camera";
		activeCamera = umCameras[activeCamName];
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

// helpers
std::shared_ptr<Mesh> Game::MeshHelper(const char* name) {
	return std::make_shared<Mesh>(name, FixPath(std::format("../../Assets/Models/{}.obj", name)).c_str());
}

void Game::EntityHelper(const char* name, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat, XMFLOAT3 translate) {
	std::shared_ptr<GameEntity> entity = std::make_shared<GameEntity>(name, mesh, mat);
	entity->GetTransform()->MoveAbsolute(translate);
	lEntities.push_back(entity);
}

std::shared_ptr<SimpleVertexShader> Game::VSHelper(const std::wstring & filename) {
	return std::make_shared<SimpleVertexShader>(Graphics::Device, Graphics::Context, FixPath(filename).c_str());
}

std::shared_ptr<SimplePixelShader> Game::PSHelper(const std::wstring & filename) {
	return std::make_shared<SimplePixelShader>(Graphics::Device, Graphics::Context, FixPath(filename).c_str());
}

// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// load meshes
	std::shared_ptr<Mesh> cube = MeshHelper("cube");
	std::shared_ptr<Mesh> cylinder = MeshHelper("cylinder");
	std::shared_ptr<Mesh> helix = MeshHelper("helix");
	std::shared_ptr<Mesh> sphere = MeshHelper("sphere");
	std::shared_ptr<Mesh> torus = MeshHelper("torus");
	std::shared_ptr<Mesh> quad = MeshHelper("quad");
	std::shared_ptr<Mesh> quad_double_sided = MeshHelper("quad_double_sided");


	// load vertex shaders
	std::shared_ptr<SimpleVertexShader> vs = VSHelper(L"VertexShader.cso");
	std::shared_ptr<SimpleVertexShader> vsSS = VSHelper(L"SpinShrinkVS.cso");

	// load pixel shaders
	std::shared_ptr<SimplePixelShader> ps = PSHelper(L"PixelShader.cso");
	std::shared_ptr<SimplePixelShader> psDbNs = PSHelper(L"DebugNormalsPS.cso");
	std::shared_ptr<SimplePixelShader> psDbUVs = PSHelper(L"DebugUVsPS.cso");
	std::shared_ptr<SimplePixelShader> psCustom = PSHelper(L"CustomPS.cso");

	// create materials
	std::shared_ptr<Material> ndbmat = std::make_shared<Material>("ndbmat", vs, psDbNs, XMFLOAT4(1, 1, 1, 1));
	std::shared_ptr<Material> uvdbmat = std::make_shared<Material>("uvdbmat", vs, psDbUVs, XMFLOAT4(1, 1, 1, 1));
	std::shared_ptr<Material> custom1 = std::make_shared<Material>("custom1", vsSS, psCustom, XMFLOAT4(1, 0, 1, 1));
	std::shared_ptr<Material> custom2 = std::make_shared<Material>("custom2", vs, psCustom, XMFLOAT4(0, 1, 0, 1));
	std::shared_ptr<Material> custom3 = std::make_shared<Material>("custom3", vsSS, psCustom, XMFLOAT4(0, 0, 1, 1));

	// create entities
	EntityHelper("cube1", cube, ndbmat, XMFLOAT3(0, 4, 0));
	EntityHelper("cylinder1", cylinder, ndbmat, XMFLOAT3(4, 4, 0));
	EntityHelper("helix1", helix, ndbmat, XMFLOAT3(8, 4, 0));
	EntityHelper("sphere1", sphere, ndbmat, XMFLOAT3(12, 4, 0));
	EntityHelper("torus1", torus, ndbmat, XMFLOAT3(16, 4, 0));
	EntityHelper("quad1", quad, ndbmat, XMFLOAT3(20, 4, 0));
	EntityHelper("quad_double_sided1", quad_double_sided, ndbmat, XMFLOAT3(24, 4, 0));

	EntityHelper("cube2", cube, uvdbmat, XMFLOAT3(0, 0, 0));
	EntityHelper("cylinder2", cylinder, uvdbmat, XMFLOAT3(4, 0, 0));
	EntityHelper("helix2", helix, uvdbmat, XMFLOAT3(8, 0, 0));
	EntityHelper("sphere2", sphere, uvdbmat, XMFLOAT3(12, 0, 0));
	EntityHelper("torus2", torus, uvdbmat, XMFLOAT3(16, 0, 0));
	EntityHelper("quad2", quad, uvdbmat, XMFLOAT3(20, 0, 0));
	EntityHelper("quad_double_sided2", quad_double_sided, uvdbmat, XMFLOAT3(24, 0, 0));

	EntityHelper("cube3", cube, custom1, XMFLOAT3(0, -4, 0));
	EntityHelper("cylinder3", cylinder, custom1, XMFLOAT3(4, -4, 0));
	EntityHelper("helix3", helix, custom2, XMFLOAT3(8, -4, 0));
	EntityHelper("sphere3", sphere, custom2, XMFLOAT3(12, -4, 0));
	EntityHelper("torus3", torus, custom2, XMFLOAT3(16, -4, 0));
	EntityHelper("quad3", quad, custom3, XMFLOAT3(20, -4, 0));
	EntityHelper("quad_double_sided3", quad_double_sided, custom3, XMFLOAT3(24, -4, 0));

}


// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	if (activeCamera) activeCamera->UpdateProjectionMatrix(Window::AspectRatio());
}


// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	activeCamera->Update(deltaTime);

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
void Game::Draw(float dt, float tt)
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
		lEntities[i]->Draw(activeCamera, dt, tt);
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
		if (ImGui::TreeNode("App Details"))
		{
			// show frame rate and window size
			ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
			ImGui::Text("Window Client Size: %dx%d", Window::Width(), Window::Height());

			// 'close' tree node
			ImGui::TreePop();
		}

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

		if (ImGui::TreeNode("World Properties"))
		{
			ImGui::ColorEdit4("Bg Color", bgColor);
			ImGui::TreePop();
		}

		// camera ui
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Camera")) {
			
			// drop down select for cameras
			// make input width smaller
			float width = ImGui::CalcItemWidth();
			ImGui::SetNextItemWidth(width * 3.0f / 4.0f);
			if (ImGui::BeginCombo("Active Camera", activeCamName.c_str())) {
				// loop through cameras map
				for (const auto& [name, cam] : umCameras) {
					bool selected = (activeCamName == name);
					if (ImGui::Selectable(name.c_str(), selected)) {
						activeCamName = name;
						activeCamera = cam;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			// interact with transform
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Transform")) {
				Transform& tCam = *activeCamera->GetTransform();
				XMFLOAT3 pos = tCam.GetPosition();
				if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&pos), 0.1f, -FLT_MAX, FLT_MAX, "%.3f")) {
					tCam.SetPosition(pos);
				}
				XMFLOAT3 rotation = tCam.GetRotation();
				rotation.x = XMConvertToDegrees(rotation.x);
				rotation.y = XMConvertToDegrees(rotation.y);
				rotation.z = XMConvertToDegrees(rotation.z);
				if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&rotation), 0.1f, -360.0f, 360.0f, "%.3f")) {
					tCam.SetRotation(XMConvertToRadians(rotation.x), 
						XMConvertToRadians(rotation.y), 
						XMConvertToRadians(rotation.z));
				}
				ImGui::TreePop();
			}

			// interact with setters
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Camera options")) {
				
				// make input width smaller
				float width = ImGui::CalcItemWidth();
				ImGui::PushItemWidth(width * 2.0f/3.0f);
				float aspectRatio = activeCamera->GetAspectRatio();
				if (ImGui::DragFloat("Aspect Ratio", &aspectRatio, 0.01f, -FLT_MAX, FLT_MAX)) {
					activeCamera->SetAspectRatio(aspectRatio);
				}
				float fov = XMConvertToDegrees(activeCamera->GetFOV());
				if (ImGui::DragFloat("FOV", &fov, 1.0f, XMConvertToDegrees(0.01f), 360)) {
					activeCamera->SetFOV(XMConvertToRadians(fov));
				}
				float orthoWidth = activeCamera->GetOrthoWidth();
				if (ImGui::DragFloat("Ortho Width", &orthoWidth, 1.0f, -FLT_MAX, FLT_MAX)) {
					activeCamera->SetOrthoWidth(orthoWidth);
				}
				float nearClip = activeCamera->GetNearClip();
				if (ImGui::DragFloat("Near Clip", &nearClip, 0.001f, 0.001f, FLT_MAX)) {
					activeCamera->SetNearClip(nearClip);
				}
				float farClip = activeCamera->GetFarClip();
				if (ImGui::DragFloat("Far Clip", &farClip, 1.0f, nearClip, FLT_MAX)) {
					activeCamera->SetFarClip(farClip);
				}
				float moveSpeed = activeCamera->GetMoveSpeed();
				if (ImGui::DragFloat("Move Speed", &moveSpeed, 0.1f, -FLT_MAX, FLT_MAX)) {
					activeCamera->SetMoveSpeed(moveSpeed);
				}
				float lookSpeed = activeCamera->GetLookSpeed();
				if (ImGui::DragFloat("Look Speed", &lookSpeed, 0.01f, -FLT_MAX, FLT_MAX)) {
					activeCamera->SetLookSpeed(lookSpeed);
				}
				float moveFactor = activeCamera->GetMoveFactor();
				if (ImGui::DragFloat("Move Factor", &moveFactor, 0.1f, -FLT_MAX, FLT_MAX)) {
					activeCamera->SetMoveFactor(moveFactor);
				}

				// Projection Type Dropdown
				const char* projectionTypes[] = { "Perspective", "Orthographic" };
				int currentProjection = static_cast<int>(activeCamera->GetProjectionType());

				if (ImGui::Combo("Projection Type", &currentProjection, projectionTypes, IM_ARRAYSIZE(projectionTypes))) {
					activeCamera->SetProjectionType(static_cast<CameraProjectionType>(currentProjection));
				}
				ImGui::PopItemWidth();
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		// mesh ui
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Entities")) {
			for (size_t i = 0; i < lEntities.size(); i++) {

				// get address for entity, mesh, and transform
				GameEntity& targetEntity = *lEntities[i].get();
				Mesh& targetMesh = *targetEntity.GetMesh().get();
				Transform& targetTransform = *targetEntity.GetTransform().get();
				Material& targetMat = *targetEntity.GetMaterial().get();

				ImGui::PushID(static_cast<UINT>(i));
				if (ImGui::TreeNode(std::format("Entity: {}", targetEntity.GetName()).c_str())) {
					if (ImGui::TreeNode(std::format("Mesh: {}", targetMesh.GetName()).c_str())) {
						ImGui::Text("triangles: %d", targetMesh.GetTriCount());
						ImGui::Text("vertices: %d", targetMesh.GetVertexCount());
						ImGui::Text("indices: %d", targetMesh.GetIndexCount());
						ImGui::TreePop();
					}

					if (ImGui::TreeNode("Transform:")) {
						// transform setters
						XMFLOAT3 pos = targetTransform.GetPosition();
						if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&pos), 0.1f, -FLT_MAX, FLT_MAX, "%.3f")) {
							targetTransform.SetPosition(pos);
						}
						XMFLOAT3 rotation = targetTransform.GetRotation();
						rotation.x = XMConvertToDegrees(rotation.x);
						rotation.y = XMConvertToDegrees(rotation.y);
						rotation.z = XMConvertToDegrees(rotation.z);
						if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&rotation), 0.1f, -360.0f, 360.0f, "%.3f")) {
							targetTransform.SetRotation(
								XMConvertToRadians(rotation.x),
								XMConvertToRadians(rotation.y),
								XMConvertToRadians(rotation.z));
						}
						XMFLOAT3 scale = targetTransform.GetScale();
						if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&scale), 0.1f, -FLT_MAX, FLT_MAX, "%.3f")) {
							targetTransform.SetScale(scale.x, scale.y, scale.z);
						}
						ImGui::TreePop();
					}

					if (ImGui::TreeNode(std::format("Material: {}", targetMat.GetName()).c_str())) {
						// color tint setter
						XMFLOAT4 colorTint = targetMat.GetColorTint();
						if (ImGui::ColorEdit4("Color Tint", reinterpret_cast<float*>(&colorTint))) {
							targetMat.SetColorTint(colorTint);
						}
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}
	// close window
	ImGui::End();

}


