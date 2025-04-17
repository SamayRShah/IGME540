#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "Lights.h"

// ImGui & simple shaders includes
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "SimpleShader/SimpleShader.h"

// assignment class includes
#include "Mesh.h"
#include "Material.h"

// d3d and std includes
#include <DirectXMath.h>
#include <WICTextureLoader.h>
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
			{"Main Camera", std::make_shared<Camera>(DirectX::XMFLOAT3(-10, 4, -12), Window::AspectRatio())},
			{"Top Ortho", std::make_shared<Camera>(DirectX::XMFLOAT3(0, 5, 0), Window::AspectRatio(), Orthographic, XM_PIDIV4, 30.0f)},
			{"Side Ortho", std::make_shared<Camera>(DirectX::XMFLOAT3(15, 0, 0), Window::AspectRatio(), Orthographic, XM_PIDIV4, 30.0f)},
			{"Top Perspective", std::make_shared<Camera>(DirectX::XMFLOAT3(0, 5, -3.5), Window::AspectRatio(), Perspective, XMConvertToRadians(100))},
			{"Side Perspective", std::make_shared<Camera>(DirectX::XMFLOAT3(15, 0, -3.5), Window::AspectRatio(), Perspective, XMConvertToRadians(60))}
		};

		umCameras["Top Ortho"]->GetTransform()->SetRotation(XM_PIDIV2, 0, 0);
		umCameras["Side Ortho"]->GetTransform()->SetRotation(0, -XM_PIDIV2, 0);
		umCameras["Top Perspective"]->GetTransform()->SetRotation(XM_PIDIV4, 0, 0);
		umCameras["Side Perspective"]->GetTransform()->SetRotation(0, -XM_PIDIV2, 0);
		umCameras["Main Camera"]->GetTransform()->SetRotation(XMConvertToRadians(16), XMConvertToRadians(27), 0);


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
	std::shared_ptr<Material> mat, XMFLOAT3 translate) {
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

std::shared_ptr<Sky> Game::SkyHelper(const char* path, std::shared_ptr<Mesh> cube,
	std::shared_ptr<SimpleVertexShader> skyVS, std::shared_ptr<SimplePixelShader> skyPS,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler, XMFLOAT3 ambientColor) {

	std::wstring wPath = std::wstring(path, path + strlen(path));
	std::shared_ptr<Sky> sky = std::make_shared<Sky>(
		FixPath(L"../../Assets/Skies/" + wPath + L"/right.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/left.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/up.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/down.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/front.png").c_str(),
		FixPath(L"../../Assets/Skies/" + wPath + L"/back.png").c_str(),
		cube, skyVS, skyPS, sampler);
	sky->SetAmbientColor(ambientColor);
	umSkies[path] = sky;
	return sky;
}

std::shared_ptr<Material> Game::MatHelperPhong(
	const char* name,
	std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<SimplePixelShader> ps,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals
) {
	XMFLOAT3 ct = XMFLOAT3(1.0f, 1.0f, 1.0f);
	std::shared_ptr<Material> mat = std::make_shared<Material>(name, vs, ps, ct, 0.0f);
	mat->AddSampler("BasicSampler", sampler);
	mat->AddTextureSRV("Albedo", albedo);
	mat->AddTextureSRV("NormalMap", normals);
	umMats[name] = mat;
	return mat;
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

// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// create sampler state
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	D3D11_SAMPLER_DESC dscSampler{};
	dscSampler .AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	dscSampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	dscSampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	dscSampler.Filter = D3D11_FILTER_ANISOTROPIC;
	dscSampler.MaxAnisotropy = 16;
	dscSampler.MaxLOD = D3D11_FLOAT32_MAX;
	Graphics::Device->CreateSamplerState(&dscSampler, sampler.GetAddressOf());

	// load textures & normal maps
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleA, cobbleN, cobbleR, cobbleM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorA, floorN, floorR, floorM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintA, paintN, paintR, paintM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedA, scratchedN, scratchedR, scratchedM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeA, bronzeN, bronzeR, bronzeM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughA, roughN, roughR, roughM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodA, woodN, woodR, woodM;

	LoadPBRTexture(L"cobblestone", cobbleA, cobbleN, cobbleR, cobbleM);
	LoadPBRTexture(L"floor", floorA, floorN, floorR, floorM);
	LoadPBRTexture(L"paint", paintA, paintN, paintR, paintM);
	LoadPBRTexture(L"scratched", scratchedA, scratchedN, scratchedR, scratchedM);
	LoadPBRTexture(L"bronze", bronzeA, bronzeN, bronzeR, bronzeM);
	LoadPBRTexture(L"rough", roughA, roughN, roughR, roughM);
	LoadPBRTexture(L"wood", woodA, woodN, woodR, woodM);

	// decal texture
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> beansSRV;
	LoadTexture(L"Base/beans.jpg", beansSRV);

	// flat normals
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> flatNSRV;
	LoadTexture(L"Base/flat_normals.png", flatNSRV);

	// phong textures with normals
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneSRV, cobblestoneNSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cushionSRV, cushionNSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockSRV, rockNSRV;
	LoadTexture(L"Base/cobblestone.png", cobblestoneSRV);
	LoadTexture(L"Base/cushion.png", cushionSRV);
	LoadTexture(L"Base/rock.png", rockSRV);
	LoadTexture(L"Base/cobblestone_normals.png", cobblestoneNSRV);
	LoadTexture(L"Base/cushion_normals.png", cushionNSRV);
	LoadTexture(L"Base/rock_normals.png", rockNSRV);

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
	std::shared_ptr<SimpleVertexShader> skyVS = VSHelper(L"SkyVS.cso");

	// load pixel shaders
	std::shared_ptr<SimplePixelShader> ps = PSHelper(L"PixelShader.cso");
	std::shared_ptr<SimplePixelShader> psPhong = PSHelper(L"PixelShaderPhong.cso");
	std::shared_ptr<SimplePixelShader> psDbNs = PSHelper(L"DebugNormalsPS.cso");
	std::shared_ptr<SimplePixelShader> psDbUVs = PSHelper(L"DebugUVsPS.cso");
	std::shared_ptr<SimplePixelShader> psDbL = PSHelper(L"DebugLightingPS.cso");
	std::shared_ptr<SimplePixelShader> psCustom = PSHelper(L"CustomPS.cso");
	std::shared_ptr<SimplePixelShader> psTexMultiply = PSHelper(L"TextureMultiplyPS.cso");
	std::shared_ptr<SimplePixelShader> skyPS = PSHelper(L"SkyPS.cso");

	std::shared_ptr<Material> ndbmat = std::make_shared<Material>("Normals Debug", vs, psDbNs, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
	std::shared_ptr<Material> uvdbmat = std::make_shared<Material>("UV Debug", vs, psDbUVs, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
	std::shared_ptr<Material> ldbmat = std::make_shared<Material>("Lighting Debug", vs, psDbL, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
	std::shared_ptr<Material> mCustom1 = std::make_shared<Material>("custom", vs, psCustom, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
	std::shared_ptr<Material> mCustom2 = std::make_shared<Material>("spinning custom", vsSS, psCustom, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
	umMats.insert({
		{"Normals Debug", ndbmat},
		{"UV Debug", uvdbmat},
		{"Lighting Debug", ldbmat},
		{"custom", mCustom1},
		{"spinning custom", mCustom2},
	});

	// phong materials
	std::shared_ptr<Material> mRock = MatHelperPhong("Rock (Phong)", vs, psPhong, sampler, rockSRV, rockNSRV);
	std::shared_ptr<Material> mCushion = MatHelperPhong("Cushion (Phong)", vs, psPhong, sampler, cushionSRV, cushionNSRV);
	std::shared_ptr<Material> mCobbleStone = MatHelperPhong("Cobblestone (Phong)", vs, psPhong, sampler, cobblestoneSRV, cobblestoneNSRV);
	std::shared_ptr<Material> mCobbleDecal = std::make_shared<Material>("Cobble Decal (Phong)", vs, psTexMultiply, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.1f);
	mCobbleDecal->AddSampler("BasicSampler", sampler);
	mCobbleDecal->AddTextureSRV("Albedo", cobblestoneSRV);
	mCobbleDecal->AddTextureSRV("DecalTexture", beansSRV);
	mCobbleDecal->AddTextureSRV("NormalMap", cobblestoneNSRV);
	umMats["Cobble Decal (Phong)"] = mCobbleDecal;

	// pbr materials
	std::shared_ptr<Material> mCobble = MatHelperPBR(
		"Cobblestone PBR", vs, ps, sampler, cobbleA, cobbleN, cobbleR, cobbleM);
	std::shared_ptr<Material> mFloor = MatHelperPBR(
		"Floor PBR", vs, ps, sampler, floorA, floorN, floorR, floorM);
	std::shared_ptr<Material> mPaint = MatHelperPBR(
		"Paint PBR", vs, ps, sampler, paintA, paintN, paintR, paintM);
	std::shared_ptr<Material> mScratched = MatHelperPBR(
		"Scratched PBR", vs, ps, sampler, scratchedA, scratchedN, scratchedR, scratchedM);
	std::shared_ptr<Material> mBronze = MatHelperPBR(
		"Bronze PBR", vs, ps, sampler, bronzeA, bronzeN, bronzeR, bronzeM);
	std::shared_ptr<Material> mRough = MatHelperPBR(
		"Rough PBR", vs, ps, sampler, roughA, roughN, roughR, roughM);
	std::shared_ptr<Material> mWood = MatHelperPBR(
		"Wood PBR", vs, ps, sampler, woodA, woodN, woodR, woodM);

	// create entities
	EntityHelper("Sphere1", sphere, mCobble, XMFLOAT3(-9, 0, 0));
	EntityHelper("Sphere2", sphere, mFloor, XMFLOAT3(-6, 0, 0));
	EntityHelper("Sphere3", sphere, mPaint, XMFLOAT3(-3, 0, 0));
	EntityHelper("Sphere4", sphere, mScratched, XMFLOAT3(0, 0, 0));
	EntityHelper("Sphere5", sphere, mBronze, XMFLOAT3(3, 0, 0));
	EntityHelper("Sphere6", sphere, mRough, XMFLOAT3(6, 0, 0));
	EntityHelper("Sphere7", sphere, mWood, XMFLOAT3(9, 0, 0));

	// create sky
	activeSky = SkyHelper("Clouds Blue", cube, skyVS, skyPS, sampler);
	activeSkyName = "Clouds Blue";
	SkyHelper("Clouds Pink", cube, skyVS, skyPS, sampler, XMFLOAT3(0.30f, 0.14f, 0.19f));
	SkyHelper("Cold Sunset", cube, skyVS, skyPS, sampler, XMFLOAT3(0.141f, 0.141f, 0.224f));
	SkyHelper("Planet", cube, skyVS, skyPS, sampler, XMFLOAT3(0.08f, 0.02f, 0.02f));
	umSkies["No Sky"] = nullptr;

	// create lights & set bg and ambient colors
	bgColor = XMFLOAT3(0, 0, 0);
	ambientColor = XMFLOAT3(0.1f, 0.15f, 0.18f);

	Light dl1= {};
	dl1.Color = XMFLOAT3(1, 0, 0);
	dl1.Type = LIGHT_TYPE_DIRECTIONAL;
	dl1.Intensity = 1;
	dl1.Direction = XMFLOAT3(1, 0, 0);
	lights.push_back(dl1);

	Light dl2 = {};
	dl2.Color = XMFLOAT3(1, 1, 1);
	dl2.Type = LIGHT_TYPE_DIRECTIONAL;
	dl2.Intensity = 1;
	dl2.Direction = XMFLOAT3(0, -1, 0);
	lights.push_back(dl2);

	Light dl3 = {};
	dl3.Color = XMFLOAT3(0, 0, 1);
	dl3.Type = LIGHT_TYPE_DIRECTIONAL;
	dl3.Intensity = 1;
	dl3.Direction = XMFLOAT3(0, 0, 1);
	lights.push_back(dl3);

	Light pl1 = {};
	pl1.Color = XMFLOAT3(1, 1, 1);
	pl1.Type = LIGHT_TYPE_POINT;
	pl1.Intensity = 1;
	pl1.Position = XMFLOAT3(15, 5, 0);
	pl1.Range = 15;
	lights.push_back(pl1);

	Light pl2 = {};
	pl2.Color = XMFLOAT3(1, 1, 1);
	pl2.Type = LIGHT_TYPE_POINT;
	pl2.Intensity = 1;
	pl2.Position = XMFLOAT3(-15, 5, 0);
	pl2.Range = 15;
	lights.push_back(pl2);

	Light sl1 = {};
	sl1.Color = XMFLOAT3(1, 1, 0);
	sl1.Type = LIGHT_TYPE_SPOT;
	sl1.Intensity = 2;
	sl1.Position = XMFLOAT3(0, 5, 0);
	sl1.Direction = XMFLOAT3(0, -1, 0);
	sl1.Range = 15;
	sl1.SpotInnerAngle = XMConvertToRadians(20);
	sl1.SpotOuterAngle = XMConvertToRadians(30);
	lights.push_back(sl1);
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
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(),	reinterpret_cast<float*>(&bgColor));
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// draw meshes
	for (size_t i = 0; i < lEntities.size(); i++) {
		lEntities[i]->GetMaterial()->GetPixelShader()->SetData(
			"lights", // The name of the (temporary) variable in the shader
			&lights[0], // The address of the data to set
			sizeof(Light) * (int)lights.size()); // The size of the data (the whole struct!) to set
		lEntities[i]->GetMaterial()->GetPixelShader()->SetInt("nLights", (int)lights.size());
		lEntities[i]->GetMaterial()->GetPixelShader()->SetFloat3("ambient", ambientColor);
		lEntities[i]->Draw(activeCamera, dt, tt);
	}

	// draw sky
	if(activeSky != nullptr)
		activeSky->Draw(activeCamera);

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
			ImGui::ColorEdit3("Bg Color", reinterpret_cast<float*>(&bgColor));
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Sky Box")) 
		{
			// drop down select for cameras
			// make input width smaller
			float width = ImGui::CalcItemWidth();
			ImGui::SetNextItemWidth(width * 3.0f / 4.0f);
			if (ImGui::BeginCombo("Active Sky", activeSkyName.c_str())) {
				// loop through cameras map
				for (const auto& [name, sky] : umSkies) {
					bool selected = (activeSkyName == name);
					if (ImGui::Selectable(name.c_str(), selected)) {
						activeSkyName = name;
						activeSky = sky;
						if (activeSky != nullptr) {
							ambientColor = sky->GetAmbientColor();
						}
						else {
							ambientColor = bgColor;
						}
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			XMFLOAT3 ambient = (activeSky != nullptr) ? activeSky->GetAmbientColor() : ambientColor;
			if (ImGui::ColorPicker3("Ambient Color", reinterpret_cast<float*>(&ambient))) {
				if(activeSky != nullptr)
					activeSky->SetAmbientColor(ambient);
				ambientColor = ambient;
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode(std::format("Lights ({})", (int)lights.size()).c_str())) {
			// Array of light type labels
			const char* lightTypeItems[] = { "Directional", "Point", "Spot" };
			for (size_t i = 0; i < lights.size(); i++) {
				Light* light = &lights[i];
				int lightType = light->Type;
				if (ImGui::TreeNode(std::format("light [{}] ({})", i, lightTypeItems[lightType]).c_str())) {
					// styling width
					float width = ImGui::CalcItemWidth();
					ImGui::PushItemWidth(width * 4.0f / 5.0f);
					// ComboBox to select light type
					if (ImGui::Combo("Light Type", &lightType, lightTypeItems, IM_ARRAYSIZE(lightTypeItems))) {
						// Update the light type in your Light object
						light->Type = lightType;
					}

					// Drag Float Inputs for Light Properties
					ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&light->Color));
					ImGui::DragFloat3("Position", reinterpret_cast<float*>(&light->Position), 0.1f, -FLT_MAX, FLT_MAX, "%.2f");
					ImGui::DragFloat3("Direction", reinterpret_cast<float*>(&light->Direction), 0.1f, -1.0f, 1.0f, "%.2f");
					ImGui::DragFloat("Range", &light->Range, 0.1f, 0.0f, FLT_MAX, "%.2f");
					ImGui::DragFloat("Intensity", &light->Intensity, 0.1f, 0.0f, FLT_MAX, "%.2f");

					// float input for inner/outer angles in degrees
					float sIA = XMConvertToDegrees(light->SpotInnerAngle);
					float sOA = XMConvertToDegrees(light->SpotOuterAngle);
					if (ImGui::DragFloat("Spot Inner Angle", &sIA, 0.1f, 0.0f, 90.0f, "%.2f")) {
						light->SpotInnerAngle = XMConvertToRadians(sIA);
					}
					if (ImGui::DragFloat("Spot Outer Angle", &sOA, 0.1f, 0.0f, 90.0f, "%.2f")) {
						light->SpotOuterAngle = XMConvertToRadians(sOA);
					}

					ImGui::DragFloat2("Padding", reinterpret_cast<float*>(&light->Padding), 0.01f, 0.0f, 1.0f, "%.2f");
					ImGui::PopItemWidth();

					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		// camera ui
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

		// materials ui
		if (ImGui::TreeNode("Materials")) {
			for (const auto& [name, mat] : umMats) {
				if (ImGui::TreeNode(mat->GetName())) {
					// textures
					if (ImGui::TreeNode("Textures")) {
						const auto& textureMap = mat->GetTextureSRVMap();
						if (textureMap.empty()) {
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red color
							ImGui::Text("No texture"); // Use a helper function for italics
							ImGui::PopStyleColor();
						}
						else {
							for (const auto& [name, texture] : textureMap) {
								ImGui::Text("%s", name.c_str());
								ImGui::Image(reinterpret_cast<ImTextureID>(texture.Get()), ImVec2(64, 64));

								std::string comboId = "##combo_" + name;
								// Show combo with placeholder names
								std::string previewLabel = "Replace " + name;
								int selectedIndex = -1;
								if (ImGui::BeginCombo(comboId.c_str(), previewLabel.c_str())) {
									for (int i = 0; i < lTextureSRVs.size(); ++i) {
										ImGui::PushID(i); // Unique ID for each item

										// selectable group to pick textrure
										ImGui::BeginGroup();
										ImGui::Image(reinterpret_cast<ImTextureID>(lTextureSRVs[i].Get()), ImVec2(64, 64));
										ImGui::SameLine();

										// Texture index as label
										std::string label = "Texture " + std::to_string(i);
										bool isSelected = (selectedIndex == i);
										if (ImGui::Selectable(label.c_str(), isSelected, 0, ImVec2(100, 64))) {
											selectedIndex = i;
											mat->ReplaceTextureSRV(name, lTextureSRVs[i]);
										}

										if (isSelected)
											ImGui::SetItemDefaultFocus();

										ImGui::EndGroup();
										ImGui::PopID();
									}
									ImGui::EndCombo();
								}
							}
						}
						ImGui::TreePop();
					}
					// color tint setter
					XMFLOAT3 colorTint = mat->GetColorTint();
					if (ImGui::ColorEdit3("Color Tint", reinterpret_cast<float*>(&colorTint))) {
						mat->SetColorTint(colorTint);
					}
					// uv setters
					XMFLOAT2 uvScale = mat->GetUvScale();
					if (ImGui::DragFloat2("uv scale", reinterpret_cast<float*>(&uvScale), 0.1f)) {
						mat->SetUvScale(uvScale);
					}
					XMFLOAT2 uvOffset = mat->GetUvOffset();
					if (ImGui::DragFloat2("uv offset", reinterpret_cast<float*>(&uvOffset), 0.1f)) {
						mat->SetUvOffset(uvOffset);
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		
		// mesh ui
		if (ImGui::TreeNode("Entities")) {
			for (size_t i = 0; i < lEntities.size(); i++) {
				// get address for entity, mesh, and transform
				GameEntity& targetEntity = *lEntities[i].get();
				Mesh& targetMesh = *targetEntity.GetMesh().get();
				Transform& targetTransform = *targetEntity.GetTransform().get();
				Material& targetMat = *targetEntity.GetMaterial().get();

				ImGui::PushID(static_cast<UINT>(i));
				if (ImGui::TreeNode(targetEntity.GetName())) {
					if (ImGui::TreeNode(std::format("Mesh: {}", targetMesh.GetName()).c_str())) {
						// style width
						float width = ImGui::CalcItemWidth();
						ImGui::SetNextItemWidth(width * 3.0f / 4.0f);
						if (ImGui::BeginCombo("Mesh", targetMesh.GetName())) {
							// loop through meshes map
							for (const auto& [name, mesh] : umMeshes) {
								bool selected = (targetMat.GetName() == name);
								if (ImGui::Selectable(name.c_str(), selected)) {
									targetEntity.SetMesh(mesh);
								}
								if (selected) {
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}
						ImGui::Text("triangles: %d", targetMesh.GetTriCount());
						ImGui::Text("vertices: %d", targetMesh.GetVertexCount());
						ImGui::Text("indices: %d", targetMesh.GetIndexCount());
						ImGui::TreePop();
					}
					if (ImGui::TreeNode(std::format("Material: {}", targetMat.GetName()).c_str())) {
						// style width
						float width = ImGui::CalcItemWidth();
						ImGui::PushItemWidth(width * 3.0f / 4.0f);
						if (ImGui::BeginCombo("Material", targetMat.GetName())) {
							// loop through cameras map
							for (const auto& [name, mat] : umMats) {
								bool selected = (targetMat.GetName() == name);
								if (ImGui::Selectable(name.c_str(), selected)) {
									targetEntity.SetMaterial(mat);
								}
								if (selected) {
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}
						// textures
						if (ImGui::TreeNode("Textures")) {
							const auto& textureMap = targetMat.GetTextureSRVMap();
							if (textureMap.empty()) {
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
								ImGui::Text("No texture");
								ImGui::PopStyleColor();
							}
							else {
								for (const auto& [name, texture] : textureMap) {
									ImGui::Text("%s", name.c_str());
									ImGui::Image(reinterpret_cast<ImTextureID>(texture.Get()), ImVec2(64, 64));
								}
							}
							ImGui::TreePop();
						}
						// color tint setter
						XMFLOAT3 colorTint = targetMat.GetColorTint();
						if (ImGui::ColorEdit3("Color Tint", reinterpret_cast<float*>(&colorTint))) {
							targetMat.SetColorTint(colorTint);
						}
						// uv setters
						XMFLOAT2 uvScale = targetMat.GetUvScale();
						if (ImGui::DragFloat2("uv scale", reinterpret_cast<float*>(&uvScale), 0.1f)) {
							targetMat.SetUvScale(uvScale);
						}
						XMFLOAT2 uvOffset = targetMat.GetUvOffset();
						if (ImGui::DragFloat2("uv offset", reinterpret_cast<float*>(&uvOffset), 0.1f)) {
							targetMat.SetUvOffset(uvOffset);
						}
						ImGui::PopItemWidth();
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


