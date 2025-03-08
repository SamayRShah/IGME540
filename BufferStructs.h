#pragma once
#include<DirectXMath.h>

struct VertexShaderExternalData
{
	DirectX::XMFLOAT4 colorTint;
	DirectX::XMFLOAT4X4 mWorld;
	DirectX::XMFLOAT4X4 mProj;
	DirectX::XMFLOAT4X4 mView;
};