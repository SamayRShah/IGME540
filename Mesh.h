#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Vertex.h"

class Mesh
{
public:

	// constructor
	Mesh(const char* name, Vertex* ptrVertices, size_t nNumVertices, unsigned int* ptrIndeces, size_t nNumIndeces);
	~Mesh();

	// public methods
	void Draw();

	// member variable return methods
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	size_t GetIndexCount();
	size_t GetVertexCount();
	const char* GetName();
	const unsigned int GetTriCount();
private:

	const char* name;

	// buffer ComPtrs
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// integers for num indeces and vertices
	size_t nNumIndeces;
	size_t nNumVertices;
};

