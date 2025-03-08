#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Vertex.h"

class Mesh
{
public:

	// constructor, wwith overload to make from file
	Mesh(const char* name, Vertex* ptrVertices, const size_t& nVertices, UINT* ptrIndices, const size_t& nIndices);
	Mesh(const char* name, const char* objFile);
	~Mesh() {};

	// public methods
	void Draw();

	// member variable return methods
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() { return vb; };
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer() { return ib; };

	const UINT GetIndexCount() const { return nIndices; };
	const UINT GetVertexCount() const { return nVertices; };
	const UINT GetTriCount() const { return nTris; };
	const char* GetName() const { return name; };

private:
	// buffer ComPtrs
	Microsoft::WRL::ComPtr<ID3D11Buffer> vb;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ib;

	// integers for num indeces and vertices
	UINT nIndices;
	UINT nVertices;
	UINT nTris;

	const char* name;

	// helper for creating buffers
	void CreateBuffers(Vertex* ptrVertices, size_t nVertices, UINT* ptrIndices, size_t nIndices);
};

