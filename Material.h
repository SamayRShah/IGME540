#pragma once
#include "SimpleShader/SimpleShader.h"
#include <DirectXMath.h>
#include <memory>
class Material
{
public:
	Material(const char* name, std::shared_ptr<SimpleVertexShader> vs, 
		std::shared_ptr<SimplePixelShader> ps, DirectX::XMFLOAT4 ct);

	const std::shared_ptr<SimplePixelShader> GetPixelShader() { return pixelShader; }
	const std::shared_ptr<SimpleVertexShader> GetVertexShader() { return vertexShader; }
	const DirectX::XMFLOAT4 GetColorTint() const { return colorTint; }
	const char* GetName() { return name; }

	void SetPixelShader(std::shared_ptr<SimplePixelShader> ps) { pixelShader = ps; }
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vs) { vertexShader = vs; }
	void SetColorTint(DirectX::XMFLOAT4 ct) { colorTint = ct; }
	void SetName(const char* name) { this->name = name; }

private:
	DirectX::XMFLOAT4 colorTint;
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	const char* name;
};

