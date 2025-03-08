# SimpleShader
A set of classes to simplify Direct3D 11 shader management, constant buffer handling and resource binding.

SimpleShader supports the following shader types:
- Vertex
- Pixel
- Geometry
- Hull
- Domain
- Compute

# Example
Turn all of this...
```C++
// Representation of constant buffer layout
struct VertexShaderExternalData
{
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;
};

// Local collection of data
VertexShaderExternalData vsData = {};
vsData.worldMatrix = transform.GetWorldMatrix();
vsData.viewMatrix	= camera->GetView();
vsData.projectionMatrix = camera->GetProjection();

// CPU-to-GPU copy
D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
context->Map(vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));
context->Unmap(vsConstantBuffer.Get(), 0);
```

Into this...
```C++
// Collect data locally using variable names from the shader!
vs->SetMatrix4x4("world", transform->GetWorldMatrix());
vs->SetMatrix4x4("view", camera->GetView());
vs->SetMatrix4x4("projection", camera->GetProjection());

// CPU-to-GPU copy when ready
vs->CopyAllBufferData();
```

# How It Works
SimpleShader uses reflection to gather information about a shader’s resources.  Internally, it builds a hash table that maps variables to their offsets in constant buffers and creates its own CPU-side buffers to match.  

For example: if you had two vertex shaders and three pixel shaders, you’d create two SimpleVertexShader and three SimplePixelShader objects.  Each of these objects would create the necessary GPU resources and CPU mappings to handle all of their constant buffers and other resources like sampler states and textures.  

When you want to send data to the GPU, you do so through the corresponding SimpleShader object.  These objects contain functions like SetFloat(), SetFloat4() and SetMatrix4x4(), which take a string corresponding to a variable in a cbuffer definition and the actual data to set. SimpleShader will look up that variable in its own internal mapping and put a copy of the data into its own CPU-side buffer.  This is analogous to filling out a CPU-side struct before the map/memcpy/unmap steps.  Once you’ve set all of the data you intend to send, call CopyAllBufferData() to perform the CPU-to-GPU copy.  This allows you to set data as often as you’d like but precisely control when the GPU copy step occurs, to minimize those copies.

In addition, since SimpleShader is digging through the compiled shader code anyway, it reverse engineers an InputLayout from each vertex shader that it loads, saving you the trouble of making one yourself.  The InputLayout for a vertex shader is then set automatically whenever you call SetShader() to bind a vertex shader.

Lastly, the library will fail silently as much as possible.  That means when an error occurs, like a shader failing to load, or a variable name not being found, the library carries on and doesn’t tell you.  The allows you to change shader code without C++ breaking in the process.  However, you can turn on error and warning reporting in the library using the following lines before loading any shaders.  You’ll then see messages in the console when something goes wrong.

```c++
ISimpleShader::ReportErrors = true;
ISimpleShader::ReportWarnings = true;
```
