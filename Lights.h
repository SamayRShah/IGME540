#pragma once
#include "DirectXMath.h"

constexpr int   MAX_LIGHTS				=	    32; 
constexpr float MAX_SPECULAR_EXPONENT   =   256.0f;
constexpr int	LIGHT_TYPE_DIRECTIONAL	=  	     0;
constexpr int	LIGHT_TYPE_POINT        =	     1;
constexpr int	LIGHT_TYPE_SPOT			=        2;

struct Light {
	int Type;
	DirectX::XMFLOAT3 Direction;
	float Range;
	DirectX::XMFLOAT3 Position;
	float Intensity;
	DirectX::XMFLOAT3 Color;
	float SpotInnerAngle;
	float SpotOuterAngle;
	DirectX::XMFLOAT2 Padding;
};