#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Vertex.h"

class Mesh
{
public:

	// constructor, with overloaded not require name
	Mesh(const char* name, Vertex* ptrVertices, const size_t& nNumVertices, UINT* ptrIndeces, const size_t& nNumIndeces);
	Mesh(Vertex* ptrVertices, const size_t& nNumVertices, UINT* ptrIndeces, const size_t &nNumIndeces);
	~Mesh();

	// public methods
	void Draw();

	// member variable return methods
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();

	const UINT GetIndexCount() const;
	const UINT GetVertexCount() const;
	const UINT GetTriCount() const;
	const char* GetName();

private:

	const char* name;

	// buffer ComPtrs
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// integers for num indeces and vertices
	UINT nNumIndeces;
	UINT nNumVertices;
	UINT nNumTris;
};

