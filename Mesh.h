#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Vertex.h"

class Mesh
{
public:

	// constructor
	Mesh(const char* name, Vertex* ptrVertices, size_t nNumVertices, UINT* ptrIndeces, size_t nNumIndeces);
	~Mesh();

	// public methods
	void Draw();

	// member variable return methods
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	UINT GetIndexCount();
	UINT GetVertexCount();
	const char* GetName();
	const UINT GetTriCount();
private:

	const char* name;

	// buffer ComPtrs
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// integers for num indeces and vertices
	UINT nNumIndeces;
	UINT nNumVertices;
};

