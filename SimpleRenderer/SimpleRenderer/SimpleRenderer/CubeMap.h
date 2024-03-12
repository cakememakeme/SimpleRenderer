#pragma once

#include "Object.h"

#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <memory>

#include "Mesh.h"

class CubeMap : public Object
{
	std::shared_ptr<Mesh> mesh;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuseSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> specularSRV;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

};

