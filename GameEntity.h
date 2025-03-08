#pragma once

#include "Mesh.h"
#include "Transform.h"
#include "Camera.h"
#include "Material.h"

#include <memory>
#include <DirectXMath.h>
#include <wrl/client.h>
class GameEntity
{
public:
	// constructor - requires mesh and material, overloaded with default name
	GameEntity(const char* name, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat);
	GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat);

	// getters
	const std::shared_ptr<Mesh> GetMesh() const { return mesh; }
	const std::shared_ptr<Transform> GetTransform() const { return transform; }
	const std::shared_ptr<Material> GetMaterial() const { return material; }
	const char* GetName() const { return name; }
	
	// setters
	void SetName(const char* name) { name = name; }
	void SetMesh(std::shared_ptr<Mesh> m) { mesh = m; }
	void SetTransform(std::shared_ptr<Transform> t) { transform = t; }
	void SetMaterial(std::shared_ptr<Material> mat) { material = mat; }

	// draw method
	void Draw(std::shared_ptr<Camera> cam, float dt, float tt);
private:
	// mesh and transform pointers
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Transform> transform;
	std::shared_ptr<Material> material;

	// name member var
	const char* name;
};

