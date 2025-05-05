#include "Game.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <format>

#include "Window.h"
#include "Input.h"
#include "Sky.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

using namespace DirectX;

// --------------------------------------------------------
// Prepare new frame for the UI
// --------------------------------------------------------
void Game::UINewFrame(float dt) {

	// Feed fresh data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = dt;
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

// main ui build
void Game::BuildUI() {

	static bool showDemoWindow;

	if (showDemoWindow) ImGui::ShowDemoWindow();

	if (ImGui::Begin("Inspector")) {
		// show frame rate and window size
		if (ImGui::CollapsingHeader("App Details"))
		{
			ImGui::Spacing();
			// show frame rate and window size
			ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
			ImGui::Text("Window Client Size: %dx%d", Window::Width(), Window::Height());

			ImGui::Spacing();
		}

		// ui options
		if (ImGui::CollapsingHeader("UI Options"))
		{
			ImGui::Spacing();
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
			if (ImGui::Button(showDemoWindow ? "Hide ImGui Demo Window" : "Show ImGui Demo Window"))
				showDemoWindow = !showDemoWindow;
			ImGui::Text("Render Target Size:");
			if (ImGui::DragFloat("##Render Target Size", &rtWidth, 2, 32, 2048)) {
				rtWidth = (rtWidth / 2) * 2;
				rtHeight = rtWidth / Window::AspectRatio();
			}
			ImGui::Spacing();
		}

		UISky();
		UILights();
		UIMaterials();
		UIEntities();
		UIShadowMap();
		UIPostProcessing();
	}
	ImGui::End();
}

// sky ui
void Game::UISky()
{
	if (ImGui::CollapsingHeader("Sky box Options")) {
		float width = ImGui::CalcItemWidth();
		ImGui::SetNextItemWidth(width * 0.75f);

		if (ImGui::BeginCombo("Active Sky", activeSkyName.c_str())) {
			for (const auto& [name, sky] : umSkies) {
				bool selected = (activeSkyName == name);
				if (ImGui::Selectable(name.c_str(), selected)) {
					activeSkyName = name;
					activeSky = sky;
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (activeSky == nullptr) {
			ImGui::Text("Background Color:");
			ImGui::ColorPicker3("##Background Color", reinterpret_cast<float*>(&bgColor));
		}
	}
}

// === Lights =====
void Game::UILights() {
	if (ImGui::CollapsingHeader("Lights")) {
		ImGui::Indent();
		const char* lightTypeItems[] = { "Directional", "Point", "Spot" };

		for (size_t i = 0; i < lights.size(); ++i) {
			Light& light = lights[i];
			const char* typeStr = lightTypeItems[light.Type];

			if (i == 0) {
				// Clickable selectable list
				if (ImGui::Selectable("Shadow Casting Light", selectedLightIndex == i)) {
					selectedLightIndex = static_cast<int>(i);
				}
			}
			else {
				// Clickable selectable list
				if (ImGui::Selectable(std::format("light [{}] ({})", i, typeStr).c_str(), selectedLightIndex == i)) {
					selectedLightIndex = static_cast<int>(i);
				}
			}
		}
		ImGui::Unindent();
	}

	// Open a separate window for the selected light
	if (selectedLightIndex >= 0 && selectedLightIndex < static_cast<int>(lights.size())) {
		Light* light = &lights[selectedLightIndex];
		std::string title = selectedLightIndex == 0 ? "Details" : std::format("Light [{}] Details", selectedLightIndex);
		ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		// Call the correct editor
		if (selectedLightIndex == 0) {
			UIEditShadowLight(light);
		}
		else {
			UIEditLightCommon(light);
		}

		if (ImGui::Button("Close")) {
			selectedLightIndex = -1;
		}

		ImGui::End();
	}
}

void Game::UIEditShadowLight(Light* light) {
	ImGui::Text("Shadow-Casting Light");

	ImGui::Text("Color");
	ImGui::ColorEdit3("##Color", reinterpret_cast<float*>(&light->Color));
	ImGui::Text("Direction");
	if (ImGui::DragFloat3("##Direction", reinterpret_cast<float*>(&light->Direction), 0.1f, -1.0f, 1.0f, "%.2f"))
		UIEditShadowMapLight(light);
	ImGui::Text("Distance");
	if(ImGui::DragFloat("##Distance", &slDistance, 0.1f, -FLT_MAX, FLT_MAX, "%.2f"))
		UIEditShadowMapLight(light);
	ImGui::Text("Up Direction");
	if(ImGui::DragFloat3("##Up Direction", reinterpret_cast<float*>(&slUpDir), 0.1f, -1.0f, 1.0f, "%.2f"))
		UIEditShadowMapLight(light);
}

void Game:: UIEditLightCommon(Light* light) {
	const char* lightTypeItems[] = { "Directional", "Point", "Spot" };

	// === General Properties ===
	if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
		int& lightType = light->Type;
		ImGui::Text("Light Type");
		if(ImGui::Combo("##LightType", &lightType, lightTypeItems, IM_ARRAYSIZE(lightTypeItems)))
			light->Type = lightType;

		ImGui::Text("Color");
		ImGui::ColorEdit3("##Color", reinterpret_cast<float*>(&light->Color));

		ImGui::Text("Intensity");
		ImGui::DragFloat("##Intensity", &light->Intensity, 0.1f, 0.0f, FLT_MAX, "%.2f");
	}

	ImGui::Spacing();

	// === Transform ===
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Position");
		ImGui::DragFloat3("##Position", reinterpret_cast<float*>(&light->Position), 0.1f, -FLT_MAX, FLT_MAX, "%.2f");

		ImGui::Text("Direction");
		ImGui::DragFloat3("##Direction", reinterpret_cast<float*>(&light->Direction), 0.1f, -1.0f, 1.0f, "%.2f");
	}

	ImGui::Spacing();

	// === Attenuation ===
	if (ImGui::CollapsingHeader("Attenuation", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Range");
		ImGui::DragFloat("##Range", &light->Range, 0.1f, 0.0f, FLT_MAX, "%.2f");
	}

	ImGui::Spacing();

	// === Spot Light Angles ===
	if (light->Type == 2 && ImGui::CollapsingHeader("Spot Angles", ImGuiTreeNodeFlags_DefaultOpen)) {
		float sIA = XMConvertToDegrees(light->SpotInnerAngle);
		float sOA = XMConvertToDegrees(light->SpotOuterAngle);

		ImGui::Text("Spot Inner Angle");
		if (ImGui::DragFloat("##SpotInnerAngle", &sIA, 0.1f, 0.0f, 90.0f, "%.2f"))
			light->SpotInnerAngle = XMConvertToRadians(sIA);

		ImGui::Text("Spot Outer Angle");
		if (ImGui::DragFloat("##SpotOuterAngle", &sOA, 0.1f, 0.0f, 90.0f, "%.2f"))
			light->SpotOuterAngle = XMConvertToRadians(sOA);
	}

	ImGui::Spacing();

	// === Misc ===
	if (ImGui::CollapsingHeader("Misc", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Padding");
		ImGui::DragFloat2("##Padding", reinterpret_cast<float*>(&light->Padding), 0.01f, 0.0f, 1.0f, "%.2f");
	}
}

void Game::UIEditShadowMapLight(Light* light) {
	XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&light->Direction));
	XMVECTOR lightPos = XMVectorScale(lightDir, slDistance);
	XMMATRIX lightView = XMMatrixLookToLH(
		lightPos,
		lightDir,
		XMVector3Normalize(XMLoadFloat3(&slUpDir)));
	XMStoreFloat4x4(&lightViewMatrix, lightView);
}

// ==== Cameras ====
void Game::UICameras() {
	if (ImGui::CollapsingHeader("Cameras")) {
		ImGui::Indent();
		int i = 0;
		for (const auto& [name, cam] : umCameras) {
			bool selected = (selectedCameraName == name);
			if (ImGui::Selectable(std::format("{}", name).c_str(), selected)) {
				selectedCameraName = name;
				activeCamName = name;
				activeCamera = cam;
			}
			++i;
		}
		ImGui::Unindent();
	}

	if (!selectedCameraName.empty() && umCameras.contains(selectedCameraName)) {
		auto& camera = umCameras[selectedCameraName];
		std::string title = std::format("Camera [{}] Details", selectedCameraName);
		ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		UIEditCamera(camera);

		if (ImGui::Button("Close")) {
			selectedCameraName.clear();
		}

		ImGui::End();
	}
}

void Game::UIEditCamera(const std::shared_ptr<Camera>& camera) {
	Transform& tCam = *camera->GetTransform();

	// === Transform ===
	UITransform(tCam);

	// === Camera Settings ===
	if (ImGui::CollapsingHeader("Camera Options", ImGuiTreeNodeFlags_DefaultOpen)) {
		float width = ImGui::CalcItemWidth();
		ImGui::PushItemWidth(width * 2.0f / 3.0f);

		float aspect = camera->GetAspectRatio();
		ImGui::Text("Aspect Ratio");
		if (ImGui::DragFloat("##AspectRatio", &aspect, 0.01f, 0.01f, 10.0f)) {
			camera->SetAspectRatio(aspect);
		}

		float fov = XMConvertToDegrees(camera->GetFOV());
		ImGui::Text("FOV");
		if (ImGui::DragFloat("##FOV", &fov, 1.0f, 1.0f, 180.0f)) {
			camera->SetFOV(XMConvertToRadians(fov));
		}

		float orthoWidth = camera->GetOrthoWidth();
		ImGui::Text("Ortho Width");
		if (ImGui::DragFloat("##OrthoWidth", &orthoWidth, 1.0f)) {
			if (orthoWidth == 0)
				orthoWidth = 1;
			camera->SetOrthoWidth(orthoWidth);
		}

		float nearClip = camera->GetNearClip();
		ImGui::Text("Near Clip");
		if (ImGui::DragFloat("##NearClip", &nearClip, 0.01f, 0.01f, FLT_MAX)) {
			camera->SetNearClip(nearClip);
		}

		float farClip = camera->GetFarClip();
		ImGui::Text("Far Clip");
		if (ImGui::DragFloat("##FarClip", &farClip, 1.0f, nearClip + 0.01f, FLT_MAX)) {
			camera->SetFarClip(farClip);
		}

		float moveSpeed = camera->GetMoveSpeed();
		ImGui::Text("Move Speed");
		if (ImGui::DragFloat("##MoveSpeed", &moveSpeed, 0.1f)) {
			camera->SetMoveSpeed(moveSpeed);
		}

		float lookSpeed = camera->GetLookSpeed();
		ImGui::Text("Look Speed");
		if (ImGui::DragFloat("##LookSpeed", &lookSpeed, 0.01f)) {
			camera->SetLookSpeed(lookSpeed);
		}

		float moveFactor = camera->GetMoveFactor();
		ImGui::Text("Move Factor");
		if (ImGui::DragFloat("##MoveFactor", &moveFactor, 0.1f)) {
			camera->SetMoveFactor(moveFactor);
		}

		const char* projectionTypes[] = { "Perspective", "Orthographic" };
		int projType = static_cast<int>(camera->GetProjectionType());
		ImGui::Text("Projection Type");
		if (ImGui::Combo("##ProjectionType", &projType, projectionTypes, IM_ARRAYSIZE(projectionTypes))) {
			camera->SetProjectionType(static_cast<CameraProjectionType>(projType));
		}

		ImGui::PopItemWidth();
	}
}

// ==== Materials ======
void Game::UIMaterials() {
	if (ImGui::CollapsingHeader("Materials")) {
		ImGui::Indent();
		int i = 0;
		for (const auto& [name, mat] : umMats) {
			bool selected = (selectedMaterialName == name);
			if (ImGui::Selectable(std::format("{}", name).c_str(), selected)) {
				selectedMaterialName = name;
			}
			++i;
		}
		ImGui::Unindent();
	}

	if (!selectedMaterialName.empty() && umMats.contains(selectedMaterialName)) {
		auto& mat = umMats[selectedMaterialName];
		std::string title = std::format("Material [{}] Details", selectedMaterialName);
		ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		UIEditMaterial(mat);

		if (ImGui::Button("Close")) {
			selectedMaterialName.clear();
		}

		ImGui::End();
	}
}

void Game::UIEditMaterial(const std::shared_ptr<Material>&mat) {
	// === Textures List ===
	if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
		const auto& textureMap = mat->GetTextureSRVMap();

		if (textureMap.empty()) {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "No textures available");
		}
		else {
			for (const auto& [texName, _] : textureMap) {
				if (ImGui::Selectable(texName.c_str())) {
					openTexturePopupName = texName;
				}
			}
		}
	}

	// === Other Sections ===
	if (ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen)) {
		XMFLOAT3 colorTint = mat->GetColorTint();
		ImGui::Text("Color Tint");
		if (ImGui::ColorEdit3("##ColorTint", reinterpret_cast<float*>(&colorTint))) {
			mat->SetColorTint(colorTint);
		}
	}

	if (ImGui::CollapsingHeader("UV Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		XMFLOAT2 uvScale = mat->GetUvScale();
		ImGui::Text("UV Scale");
		if (ImGui::DragFloat2("##UvScale", reinterpret_cast<float*>(&uvScale), 0.1f)) {
			mat->SetUvScale(uvScale);
		}

		XMFLOAT2 uvOffset = mat->GetUvOffset();
		ImGui::Text("UV Offset");
		if (ImGui::DragFloat2("##UvOffset", reinterpret_cast<float*>(&uvOffset), 0.1f)) {
			mat->SetUvOffset(uvOffset);
		}
	}

	// === Texture Popup ===
	if (!openTexturePopupName.empty()) {
		UIEditTextureMap(mat, openTexturePopupName);
	}
}

void Game::UIEditTextureMap(const std::shared_ptr<Material>& mat, const std::string& texName) {
	std::string windowTitle = texName + " Texture Map";
	ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	const auto& currentSRV = mat->GetTextureSRVMap()[texName];
	if (currentSRV) {
		ImGui::Text("Current Texture");
		ImGui::Image(reinterpret_cast<ImTextureID>(currentSRV.Get()), ImVec2(64, 64));
	}
	else {
		ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No texture bound");
	}

	// Texture selection combo
	ImGui::Text("Replace With:");
	if (ImGui::BeginCombo("##TextureCombo", "Select Texture")) {
		for (int i = 0; i < static_cast<int>(lTextureSRVs.size()); ++i) {
			ImGui::PushID(i);

			ImGui::BeginGroup();

			// Render the image and label without forcing same line
			ImGui::Image(reinterpret_cast<ImTextureID>(lTextureSRVs[i].Get()), ImVec2(64, 64));

			ImGui::SameLine();

			// Add some vertical space then render label
			std::string label = "Texture " + std::to_string(i);
			if (ImGui::Selectable(label.c_str())) {
				mat->ReplaceTextureSRV(texName, lTextureSRVs[i]);
			}

			ImGui::Separator();

			ImGui::EndGroup();

			ImGui::PopID();
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("Close")) {
		openTexturePopupName.clear();
	}

	ImGui::End();
}

// ==== Entities =======
void Game::UIEntities() {
	if (ImGui::CollapsingHeader("Entities")) {
		ImGui::Indent();
		for (int i = 0; i < lEntities.size(); i++) {
			const std::string& name = lEntities[i]->GetName();
			bool selected = (selectedEntityIndex == i);
			if (ImGui::Selectable(std::format("{}", name).c_str(), selected)) {
				selectedEntityIndex = i;
			}
		}
		ImGui::Unindent();
	}

	if (selectedEntityIndex >= 0 && selectedEntityIndex < lEntities.size()) {
		auto& entity = lEntities[selectedEntityIndex];

		std::string title = std::format("Entity [{}] Details", entity->GetName());
		ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		UIEntityDetails(entity);

		if (ImGui::Button("Close")) {
			selectedEntityIndex = -1;
		}

		ImGui::End();
	}
}
void Game::UIEntityDetails(std::shared_ptr<GameEntity> entity) {
	if (!entity) return;

	std::shared_ptr<Mesh> mesh = entity->GetMesh();
	std::shared_ptr<Material> mat = entity->GetMaterial();
	std::shared_ptr<Transform> trans = entity->GetTransform();

	if (mesh) {
		UIMesh(mesh);
		entity->SetMesh(mesh);
	}

	if (mat) {
		UIMaterial(mat);
		entity->SetMaterial(mat);
	}

	if (trans) {
		UITransform(*trans);
	}
}
void Game::UITransform(Transform& transform) {
	if (ImGui::CollapsingHeader("Transform")) {
		XMFLOAT3 pos = transform.GetPosition();
		ImGui::Text("Position");
		if (ImGui::DragFloat3("##Position", reinterpret_cast<float*>(&pos), 0.1f)) {
			transform.SetPosition(pos);
		}

		XMFLOAT3 rot = transform.GetRotation();
		rot.x = XMConvertToDegrees(rot.x);
		rot.y = XMConvertToDegrees(rot.y);
		rot.z = XMConvertToDegrees(rot.z);
		ImGui::Text("Rotation");
		if (ImGui::DragFloat3("##Rotation", reinterpret_cast<float*>(&rot), 0.1f, -360.0f, 360.0f)) {
			transform.SetRotation(
				XMConvertToRadians(rot.x),
				XMConvertToRadians(rot.y),
				XMConvertToRadians(rot.z)
			);
		}

		ImGui::Text("Scale");
		XMFLOAT3 scale = transform.GetScale();
		if (ImGui::DragFloat3("##Scale", reinterpret_cast<float*>(&scale), 0.1f, -FLT_MAX, FLT_MAX, "%.3f")) {
			transform.SetScale(scale.x, scale.y, scale.z);
		}
	}
}
void Game::UIMesh(std::shared_ptr<Mesh>& targetMesh) {
	std::string meshName = targetMesh->GetName();
	std::string label = std::format("Mesh: {}", meshName);

	if (ImGui::CollapsingHeader(label.c_str())) {
		float width = ImGui::CalcItemWidth();
		ImGui::SetNextItemWidth(width * 0.75f);

		if (ImGui::BeginCombo("Mesh", meshName.c_str())) {
			for (const auto& [name, mesh] : umMeshes) {
				bool selected = (meshName == name);
				if (ImGui::Selectable(name.c_str(), selected)) {
					targetMesh = mesh;
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// Pop-out for mesh details
		std::string windowLabel = std::format("Mesh Details: {}", meshName);
		if (ImGui::Button("Open Mesh Info")) {
			ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_FirstUseEver);
			ImGui::OpenPopup(windowLabel.c_str());
		}
		if (ImGui::BeginPopup(windowLabel.c_str())) {
			ImGui::Text("Triangles: %d", targetMesh->GetTriCount());
			ImGui::Text("Vertices: %d", targetMesh->GetVertexCount());
			ImGui::Text("Indices: %d", targetMesh->GetIndexCount());
			ImGui::EndPopup();
		}
	}
}
void Game::UIMaterial(std::shared_ptr<Material>& targetMat)
{
	if (!targetMat) return;

	std::string currentName = targetMat->GetName();

	if (ImGui::CollapsingHeader("Material")) {
		float width = ImGui::CalcItemWidth();
		ImGui::PushItemWidth(width * 0.75f);

		ImGui::Text("Material:");
		if (ImGui::BeginCombo("##Material CB", currentName.c_str())) {
			for (const auto& [name, mat] : umMats) {
				bool selected = (name == currentName);
				if (ImGui::Selectable(name.c_str(), selected)) {
					targetMat = mat;
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();

		if (ImGui::Button("Edit Material")) {
			selectedMaterialName = currentName;
		}
	}
}

// ====== Shadow Map =========
void Game::UIShadowMap() {
	if (ImGui::CollapsingHeader("Shadow Map")) {
		ImGui::Image(reinterpret_cast<ImTextureID>(shadowSRV.Get()), ImVec2(rtWidth, rtWidth));

		ImGui::Text("Projection Size:");
		if (ImGui::SliderFloat("##Size", &lightProjectionSize, 0.5, 100)) {
			ResizeShadowMap();
		}

		ImGui::Text("Shadow Map Resolution (px):");
		if (ImGui::SliderInt("##Resolution", &shadowMapResolution, 2, 2048)) {
			shadowMapResolution = (shadowMapResolution / 2) * 2;
			CreateShadowMapResources();
		}
	}
}

// ====== Post Processing ====
void Game::UIPostProcessing() {
	if (ImGui::CollapsingHeader("Post Processing Effects")) {
		ImGui::Indent();

		// Selectable effects
		const char* effects[] = { "Blur", "Chromatic Aberration" };
		for (int i = 0; i < 2; i++) {
			bool selected = (selectedPostProcessIndex == i);
			if (ImGui::Selectable(effects[i], selected)) {
				selectedPostProcessIndex = i;
			}
		}

		if (ImGui::Button("Show Render Pass")) {
			showRenderPasses = true;
		}

		UIRenderPasses();

		ImGui::Unindent();
	}

	// Detail window for selected effect
	if (selectedPostProcessIndex >= 0) {
		std::string title;
		switch (selectedPostProcessIndex) {
			case 0: title = "Effect [Blur] Details"; break;
			case 1: title = "Effect [Chromatic Aberration] Details"; break;
		}

		ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		switch (selectedPostProcessIndex) {
			case 0: UIDetailsBlur(); break;
			case 1: UIDetailsChromaticAberration(); break;
		}

		if (ImGui::Button("Close")) {
			selectedPostProcessIndex = -1;
		}

		ImGui::End();
	}
}

void Game::UIRenderPasses() {
	if (!showRenderPasses) return;

	if(ImGui::Begin("Render Passes")) {
		ImGui::Text("Shadow Map:");
		ImGui::Image(reinterpret_cast<ImTextureID>(shadowSRV.Get()), ImVec2(rtWidth, rtWidth));

		ImGui::Text("Before Blur:");
		ImGui::Image(reinterpret_cast<ImTextureID>(ppBlurSRV.Get()), ImVec2(rtWidth, rtHeight));

		ImGui::Text("Before Chromatic aberration:");
		ImGui::Image(reinterpret_cast<ImTextureID>(ppChromaticSRV.Get()), ImVec2(rtWidth, rtHeight));
		
		// button to close pipeline
		if (ImGui::Button("Close")) {
			showRenderPasses = false;
		}
	}
	ImGui::End();
}
void Game::UIDetailsBlur() {
	ImGui::Text("Blur Radius:");
	ImGui::SliderInt("##Blur Radius", &ppBlurRadius, 0, 25);

	ImGui::Text("Before Blur:");
	ImGui::Image(reinterpret_cast<ImTextureID>(ppBlurSRV.Get()), ImVec2(rtWidth, rtHeight));

	ImGui::Text("Blur SRV Output:");
	ImGui::Image(reinterpret_cast<ImTextureID>(ppChromaticSRV.Get()), ImVec2(rtWidth, rtHeight));
}

void Game::UIDetailsChromaticAberration() {
	ImGui::Text("Color Sampling Offsets:");
	ImGui::DragFloat3("##Color sampling offsets", reinterpret_cast<float*>(&ppChromaticOffsets), 0.001f);

	ImGui::Text("Before Abberation:");
	ImGui::Image(reinterpret_cast<ImTextureID>(ppChromaticSRV.Get()), ImVec2(rtWidth, rtHeight));
}