#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "Lights.h"
#include "Mesh.h"
#include "Material.h"

// ImGui & simple shaders includes
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "SimpleShader/SimpleShader.h"

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

	// setup shadow map
	CreateShadowMapResources();
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

void Game::CreateShadowMapResources()
{
	// describe and create shadowmap
	// Create the actual texture that will be the shadow map
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapResolution; // Ideally a power of 2 (like 1024)
	shadowDesc.Height = shadowMapResolution; // Ideally a power of 2 (like 1024)
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	Graphics::Device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

	// Create the depth/stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	Graphics::Device->CreateDepthStencilView(
		shadowTexture.Get(),
		&shadowDSDesc,
		shadowDSV.GetAddressOf());

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	Graphics::Device->CreateShaderResourceView(
		shadowTexture.Get(),
		&srvDesc,
		shadowSRV.GetAddressOf());

	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Min. precision units, not world units!
	shadowRastDesc.SlopeScaledDepthBias = 1.0f; // Bias more based on slope
	Graphics::Device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f; // Only need the first component
	Graphics::Device->CreateSamplerState(&shadowSampDesc, &shadowSampler);
}

// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// create sampler state
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	D3D11_SAMPLER_DESC dscSampler{};
	dscSampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	dscSampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	dscSampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	dscSampler.Filter = D3D11_FILTER_ANISOTROPIC;
	dscSampler.MaxAnisotropy = 16;
	dscSampler.MaxLOD = D3D11_FLOAT32_MAX;
	Graphics::Device->CreateSamplerState(&dscSampler, sampler.GetAddressOf());

	// load textures, make entities
	{
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

		// load meshes
		std::shared_ptr<Mesh> cube, cylinder, helix, sphere, torus, quad, quad_double_sided;
		cube = MeshHelper("cube");
		cylinder = MeshHelper("cylinder");
		helix = MeshHelper("helix");
		sphere = MeshHelper("sphere");
		torus = MeshHelper("torus");
		quad = MeshHelper("quad");
		quad_double_sided = MeshHelper("quad_double_sided");

		// load vertex shaders
		std::shared_ptr<SimpleVertexShader> vs, vsSS, skyVS;
		vs = VSHelper(L"VertexShader.cso");
		vsSS = VSHelper(L"SpinShrinkVS.cso");
		skyVS = VSHelper(L"SkyVS.cso");
		shadowVS = std::make_shared<SimpleVertexShader>(Graphics::Device, Graphics::Context, FixPath(L"ShadowMapVS.cso").c_str());

		// load pixel shaders
		std::shared_ptr<SimplePixelShader> ps, psDbNs, psDbUVs, psDbL, psCustom, psTexMultiply, skyPS;
		ps = PSHelper(L"PixelShader.cso");
		psDbNs = PSHelper(L"DebugNormalsPS.cso");
		psDbUVs = PSHelper(L"DebugUVsPS.cso");
		psDbL = PSHelper(L"DebugLightingPS.cso");
		psCustom = PSHelper(L"CustomPS.cso");
		psTexMultiply = PSHelper(L"TextureMultiplyPS.cso");
		skyPS = PSHelper(L"SkyPS.cso");

		std::shared_ptr<Material> ndbmat, uvdbmat, ldbmat, mCustom1, mCustom2;
		ndbmat = std::make_shared<Material>("Normals Debug", vs, psDbNs, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
		uvdbmat = std::make_shared<Material>("UV Debug", vs, psDbUVs, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
		ldbmat = std::make_shared<Material>("Lighting Debug", vs, psDbL, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
		mCustom1 = std::make_shared<Material>("custom", vs, psCustom, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
		mCustom2 = std::make_shared<Material>("spinning custom", vsSS, psCustom, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f);
		umMats.insert({
			{"Normals Debug", ndbmat},
			{"UV Debug", uvdbmat},
			{"Lighting Debug", ldbmat},
			{"custom", mCustom1},
			{"spinning custom", mCustom2},
			});

		// pbr materials
		std::shared_ptr<Material> mCobble, mFloor, mPaint, mScratched, mBronze, mRough, mWood,
			mCobbleDecal, mFloorDecal, mPaintDecal, mScratchedDecal, mBronzeDecal, mRoughDecal, mWoodDecal;
		mCobble = MatHelperPBR(
			"Cobblestone PBR", vs, ps, sampler, cobbleA, cobbleN, cobbleR, cobbleM);
		mFloor = MatHelperPBR(
			"Floor PBR", vs, ps, sampler, floorA, floorN, floorR, floorM);
		mPaint = MatHelperPBR(
			"Paint PBR", vs, ps, sampler, paintA, paintN, paintR, paintM);
		mScratched = MatHelperPBR(
			"Scratched PBR", vs, ps, sampler, scratchedA, scratchedN, scratchedR, scratchedM);
		mBronze = MatHelperPBR(
			"Bronze PBR", vs, ps, sampler, bronzeA, bronzeN, bronzeR, bronzeM);
		mRough = MatHelperPBR(
			"Rough PBR", vs, ps, sampler, roughA, roughN, roughR, roughM);
		mWood = MatHelperPBR(
			"Wood PBR", vs, ps, sampler, woodA, woodN, woodR, woodM);

		mCobbleDecal = MatHelperDecalPBR(
			"Cobblestone Decal PBR", vs, psTexMultiply, sampler, cobbleA, beansSRV, cobbleN, cobbleR, cobbleM);
		mFloorDecal = MatHelperDecalPBR(
			"Floor Decal PBR", vs, psTexMultiply, sampler, floorA, beansSRV, floorN, floorR, floorM);
		mPaintDecal = MatHelperDecalPBR(
			"Paint Decal PBR", vs, psTexMultiply, sampler, paintA, beansSRV, paintN, paintR, paintM);
		mScratchedDecal = MatHelperDecalPBR(
			"Scratched Decal PBR", vs, psTexMultiply, sampler, scratchedA, beansSRV, scratchedN, scratchedR, scratchedM);
		mBronzeDecal = MatHelperDecalPBR(
			"Bronze Decal PBR", vs, psTexMultiply, sampler, bronzeA, beansSRV, bronzeN, bronzeR, bronzeM);
		mRoughDecal = MatHelperDecalPBR(
			"Rough Decal PBR", vs, psTexMultiply, sampler, roughA, beansSRV, roughN, roughR, roughM);
		mWoodDecal = MatHelperDecalPBR(
			"Wood Decal PBR", vs, psTexMultiply, sampler, woodA, beansSRV, woodN, woodR, woodM);

		// create entities
		EntityHelper("Sphere1", sphere, mCobble, XMFLOAT3(-9, 0, 0));
		EntityHelper("Sphere2", sphere, mFloor, XMFLOAT3(-6, 0, 0));
		EntityHelper("Sphere3", sphere, mPaint, XMFLOAT3(-3, 0, 0));
		EntityHelper("Sphere4", sphere, mScratched, XMFLOAT3(0, 0, 0));
		EntityHelper("Sphere5", sphere, mBronze, XMFLOAT3(3, 0, 0));
		EntityHelper("Sphere6", sphere, mRough, XMFLOAT3(6, 0, 0));
		EntityHelper("Sphere7", sphere, mWood, XMFLOAT3(9, 0, 0));
		EntityHelper("Sphere8", sphere, mCobbleDecal, XMFLOAT3(-9, 3, 0));
		EntityHelper("Sphere9", sphere, mFloorDecal, XMFLOAT3(-6, 3, 0));
		EntityHelper("Sphere10", sphere, mPaintDecal, XMFLOAT3(-3, 3, 0));
		EntityHelper("Sphere11", sphere, mScratchedDecal, XMFLOAT3(0, 3, 0));
		EntityHelper("Sphere12", sphere, mBronzeDecal, XMFLOAT3(3, 3, 0));
		EntityHelper("Sphere13", sphere, mRoughDecal, XMFLOAT3(6, 3, 0));
		EntityHelper("Sphere14", sphere, mWoodDecal, XMFLOAT3(9, 3, 0));
		EntityHelper("Floor", cube, mWood, XMFLOAT3(0, -5, 0), XMFLOAT3(20, 1, 20));

		// create sky
		umSkies["No Sky"] = nullptr;
		activeSky = SkyHelper("Clouds Blue", cube, skyVS, skyPS, sampler);
		activeSkyName = "Clouds Blue";
		SkyHelper("Clouds Pink", cube, skyVS, skyPS, sampler);
		SkyHelper("Cold Sunset", cube, skyVS, skyPS, sampler);
		SkyHelper("Planet", cube, skyVS, skyPS, sampler);
	}

	{
		//
	}

	// lights setup
	{
		bgColor = XMFLOAT3(0, 0, 0);

		Light dl1 = {};
		dl1.Color = XMFLOAT3(1, 1, 1);
		dl1.Type = LIGHT_TYPE_DIRECTIONAL;
		dl1.Intensity = 1;
		dl1.Direction = XMFLOAT3(0, -1.0f, 0);
		dl1.Position = XMFLOAT3(0, -15.0f, 0);
		lights.push_back(dl1);

		XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&dl1.Direction));
		XMVECTOR lightPos = XMVectorScale(lightDir, slDistance);
		// store direction and projection for the light
		XMMATRIX lightView = XMMatrixLookToLH(
			lightPos,
			lightDir,
			XMVector3Normalize(XMLoadFloat3(&slUpDir))); // Up: World up vector (Z axis for looking straight down)
		XMStoreFloat4x4(&lightViewMatrix, lightView);
		XMMATRIX lightProjection = XMMatrixOrthographicLH(
			lightProjectionSize,
			lightProjectionSize,
			1.0f,
			100.0f);
		XMStoreFloat4x4(&lightProjectionMatrix, lightProjection);

		Light dl2 = {};
		dl2.Color = XMFLOAT3(1, 0, 0);
		dl2.Type = LIGHT_TYPE_DIRECTIONAL;
		dl2.Intensity = 1;
		dl2.Direction = XMFLOAT3(1, 0, 0);
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

	// post process setup
	{

		// set post process vs/ps
		ppVS = VSHelper(L"PostProcessVS.cso");
		ppBlurPS = PSHelper(L"PPBoxBlurPS.cso");
		ppChromaticPS = PSHelper(L"PPChromaticAberration.cso");

		// post process sampler
		D3D11_SAMPLER_DESC ppSampDesc = {};
		ppSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		ppSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		ppSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		ppSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		ppSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		Graphics::Device->CreateSamplerState(&ppSampDesc, ppSampler.GetAddressOf());

		// Create the Render Target View
		ResizePostProcessResources();
	}
}

// helper to resize shadow map
void Game::ResizeShadowMap() {
	XMMATRIX lightProjection = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		1.0f,
		100.0f);
	XMStoreFloat4x4(&lightProjectionMatrix, lightProjection);
}

void Game::EditShadowMapLight(Light light, float distance) {
	XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&light.Direction));
	XMVECTOR lightPos = XMVectorScale(lightDir, distance);
	XMMATRIX lightView = XMMatrixLookToLH(
		lightPos, // Position: "Backing up" 20 units from origin
		lightDir, // Direction: light's direction
		XMVector3Normalize(XMLoadFloat3(&slUpDir)));
	XMStoreFloat4x4(&lightViewMatrix, lightView);
}

// helper to resize render targets
void Game::ResizePostProcessResources()
{
	ppBlurSRV.Reset();
	ppBlurRTV.Reset();
	ppChromaticSRV.Reset();
	ppChromaticRTV.Reset();

	// resize render target
	// Describe the texture we're creating
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = Window::Width();
	textureDesc.Height = Window::Height();
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexBlur;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexChromatic;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexChromaticOutput;

	Graphics::Device->CreateTexture2D(&textureDesc, 0, ppTexBlur.GetAddressOf());
	Graphics::Device->CreateTexture2D(&textureDesc, 0, ppTexChromatic.GetAddressOf());
	Graphics::Device->CreateTexture2D(&textureDesc, 0, ppTexChromaticOutput.GetAddressOf());

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	Graphics::Device->CreateRenderTargetView(
		ppTexBlur.Get(),
		&rtvDesc,
		ppBlurRTV.ReleaseAndGetAddressOf());
	Graphics::Device->CreateRenderTargetView(
		ppTexChromatic.Get(),
		&rtvDesc,
		ppChromaticRTV.ReleaseAndGetAddressOf());

	// Create the Shader Resource View
	Graphics::Device->CreateShaderResourceView(
		ppTexBlur.Get(),
		0,
		ppBlurSRV.ReleaseAndGetAddressOf());
	Graphics::Device->CreateShaderResourceView(
		ppTexChromatic.Get(),
		0,
		ppChromaticSRV.ReleaseAndGetAddressOf());
}

// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	if (activeCamera) activeCamera->UpdateProjectionMatrix(Window::AspectRatio());
	if (Graphics::Device) ResizePostProcessResources();
	rtHeight = rtWidth / Window::AspectRatio();
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
		ID3D11ShaderResourceView* nullSRVs[128] = {};
		Graphics::Context->PSSetShaderResources(0, 128, nullSRVs);

		// Clear buffers (erase what's on screen)
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(), reinterpret_cast<float*>(&bgColor));
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// shadow mapping
	{
		// set render state
		Graphics::Context->RSSetState(shadowRasterizer.Get());

		// clear depth stencil
		Graphics::Context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		// setup output merger state
		ID3D11RenderTargetView* nullRTV{};
		Graphics::Context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());

		// clear pixel shader
		Graphics::Context->PSSetShader(0, 0, 0);

		// change viewport
		D3D11_VIEWPORT viewport = {};
		viewport.Width = (float)shadowMapResolution;
		viewport.Height = (float)shadowMapResolution;
		viewport.MaxDepth = 1.0f;
		Graphics::Context->RSSetViewports(1, &viewport);

		// entity render loop
		shadowVS->SetShader();
		shadowVS->SetMatrix4x4("view", lightViewMatrix);
		shadowVS->SetMatrix4x4("projection", lightProjectionMatrix);
		// Loop and draw all entities
		for (auto& e : lEntities)
		{
			shadowVS->SetMatrix4x4("world", e->GetTransform()->GetWorldMatrix());
			shadowVS->CopyAllBufferData();
			e->GetMesh()->Draw();
		}

		// reset pipeline
		viewport.Width = (float)Window::Width();
		viewport.Height = (float)Window::Height();
		Graphics::Context->RSSetViewports(1, &viewport);
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());
		Graphics::Context->RSSetState(0);
	}

	// pre rendering
	{
		// Clear post processing buffers
		Graphics::Context->ClearRenderTargetView(ppBlurRTV.Get(), reinterpret_cast<float*>(&bgColor));
		Graphics::Context->ClearRenderTargetView(ppChromaticRTV.Get(), reinterpret_cast<float*>(&bgColor));

		// set 1st process as render target
		Graphics::Context->OMSetRenderTargets(1, ppBlurRTV.GetAddressOf(), Graphics::DepthBufferDSV.Get());
	}

	// render
	{
		// draw meshes
		for (size_t i = 0; i < lEntities.size(); i++) {
			std::shared_ptr<SimpleVertexShader> vs = lEntities[i]->GetMaterial()->GetVertexShader();
			vs->SetMatrix4x4("mViewLight", lightViewMatrix);
			vs->SetMatrix4x4("mProjLight", lightProjectionMatrix);

			std::shared_ptr<SimplePixelShader> ps = lEntities[i]->GetMaterial()->GetPixelShader();
			ps->SetData(
				"lights", // The name of the (temporary) variable in the shader
				&lights[0], // The address of the data to set
				sizeof(Light) * (int)lights.size()); // The size of the data (the whole struct!) to set
			ps->SetInt("nLights", (int)lights.size());
			ps->SetShaderResourceView("ShadowMap", shadowSRV);
			ps->SetSamplerState("ShadowSampler", shadowSampler);
			lEntities[i]->Draw(activeCamera, dt, tt);
		}

		// draw sky
		if (activeSky != nullptr) {
			if (activeCamera->GetProjectionType() == CameraProjectionType::Orthographic) {
				activeCamera->SetProjectionType(CameraProjectionType::Perspective);
				activeSky->Draw(activeCamera);
				activeCamera->SetProjectionType(CameraProjectionType::Orthographic);
			}
			else {
				activeSky->Draw(activeCamera);
			}
			//activeSky->Draw(activeCamera);
		}
	}

	// post processing
	{
		// Set back buffer to next PP and activate vert shader
		Graphics::Context->OMSetRenderTargets(1, ppChromaticRTV.GetAddressOf(), 0);
		ppVS->SetShader();

		// blur
		{
			// set resources
			ppBlurPS->SetShader();
			ppBlurPS->SetInt("blurRadius", ppBlurRadius);
			ppBlurPS->SetFloat("pixelWidth", 1.0f / (float)Window::Width());
			ppBlurPS->SetFloat("PixelHeight", 1.0f / (float)Window::Height());
			ppBlurPS->SetShaderResourceView("Pixels", ppBlurSRV.Get());
			ppBlurPS->SetSamplerState("ClampSampler", ppSampler.Get());
			ppBlurPS->CopyAllBufferData();
			Graphics::Context->Draw(3, 0);
		}

		// chromatic aberration
		{
			// set back buffer
			Graphics::Context->OMSetRenderTargets(1, Graphics::BackBufferRTV.GetAddressOf(), 0);
			
			// set resources
			ppChromaticPS->SetShader();
			ppChromaticPS->SetFloat3("offsets", ppChromaticOffsets);
			ppChromaticPS->SetFloat2("mousePos", XMFLOAT2((float)Input::GetMouseX(), (float)Input::GetMouseY()));
			ppChromaticPS->SetFloat2("textureSize", XMFLOAT2((float)Window::Width(), (float)Window::Height()));
			ppChromaticPS->SetShaderResourceView("Pixels", ppChromaticSRV.Get());
			ppChromaticPS->SetSamplerState("ClampSampler", ppSampler.Get());
			ppChromaticPS->CopyAllBufferData();
			Graphics::Context->Draw(3, 0);
		}
	}

	// UI at the end
	{
		// prepare ImGui buffers
		ImGui::Render(); // Turns this frame’s UI into renderable triangles
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen
	}

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




