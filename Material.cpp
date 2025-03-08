#include "Material.h"
using namespace DirectX;

Material::Material(const char* name, std::shared_ptr<SimpleVertexShader> vs,
	std::shared_ptr<SimplePixelShader> ps, DirectX::XMFLOAT4 ct)
	: name(name), pixelShader(ps), vertexShader(vs), colorTint(ct)
{
}
