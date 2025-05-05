#pragma once  
#include <d3d11.h>  
#include <string>  

class PostProcessPass {
public:
	virtual void Execute(
		ID3D11DeviceContext* ctx,
		ID3D11ShaderResourceView* inputSRV,
		ID3D11RenderTargetView* outputRTV,
		float width, float height) = 0;
	virtual std::string GetName() const = 0;
	virtual ~PostProcessPass() {}
};