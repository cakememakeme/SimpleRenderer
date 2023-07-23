#pragma once

#include "IRenderer.h"

#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl.h>

#include <string>
#include <iostream>

struct ModelViewProjectionConstantBuffer 
{
	DirectX::SimpleMath::Matrix Model;
	DirectX::SimpleMath::Matrix View;
	DirectX::SimpleMath::Matrix Projection;
};

class D3d11Renderer : public IRenderer
{
	bool bUsePerspectiveProjection = true;
	DirectX::SimpleMath::Vector3 modelTranslation = DirectX::SimpleMath::Vector3(0.0f);
	DirectX::SimpleMath::Vector3 modelRotation = DirectX::SimpleMath::Vector3(0.0f);
	DirectX::SimpleMath::Vector3 modelScaling = DirectX::SimpleMath::Vector3(0.5f);
	DirectX::SimpleMath::Vector3 viewEyePos = { 0.0f, 0.0f, -2.0f };
	DirectX::SimpleMath::Vector3 viewEyeDir = { 0.0f, 0.0f, 1.0f };
	DirectX::SimpleMath::Vector3 viewUp = { 0.0f, 1.0f, 0.0f };
	float projFovAngleY = 70.0f;
	float nearZ = 0.01f;
	float farZ = 15.0f;
	float aspect = 0.0f;

	// 윈도우 핸들
	HWND mainWindowHandle;
	int width;
	int height;

	D3D11_VIEWPORT screenViewport;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	//다른 스왑체인들 써도 문제가 생긴다... 왜이러는거야 대체
	//Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	// 여기선 ComPtr를 안쓰면 오히려 문제가 생긴다 -_-;
	//ID3D11RenderTargetView* renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;

	ModelViewProjectionConstantBuffer constantBufferData;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
	UINT indexCount;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> colorVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> colorPixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> colorInputLayout;

public:
	D3d11Renderer();
	virtual ~D3d11Renderer();


	// IRenderer을(를) 통해 상속됨
	virtual bool Create() override;

	virtual bool Initialize(HWND mainWindow, const int bufferWidth, const int bufferHeight) override;

	virtual bool SetObjects(std::shared_ptr<std::vector<std::shared_ptr<Object>>> receivedObjects) override;

	virtual void Update() override;

	virtual void Render() override;

	virtual void OnPostRender() override;

	virtual bool Reset() override;

private:
	bool initDirect3D();
	bool initGui();

	template <typename T_VERTEX>
	bool createVertexBuffer(const std::vector<T_VERTEX>& vertices, Microsoft::WRL::ComPtr<ID3D11Buffer>& vertexBuffer) 
	{
		// D3D11_USAGE enumeration (d3d11.h)
		// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 초기화 후 변경X
		bufferDesc.ByteWidth = UINT(sizeof(T_VERTEX) * vertices.size());
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0; // 0 if no CPU access is necessary.
		bufferDesc.StructureByteStride = sizeof(T_VERTEX);

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 }; // MS 예제에서 초기화하는 방식
		vertexBufferData.pSysMem = vertices.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;

		const HRESULT hr = device->CreateBuffer(&bufferDesc, &vertexBufferData, vertexBuffer.GetAddressOf());
		if (FAILED(hr)) 
		{
			std::cout << "CreateBuffer() failed. " << std::hex << hr << std::endl;
			return false;
		};

		return true;
	}

	template <typename T_CONSTANT>
	bool createConstantBuffer(const T_CONSTANT& constantBufferData, Microsoft::WRL::ComPtr<ID3D11Buffer>& constantBuffer)
	{
		D3D11_BUFFER_DESC cbDesc;
		cbDesc.ByteWidth = sizeof(constantBufferData);
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = &constantBufferData;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		if (FAILED(device->CreateBuffer(&cbDesc, &InitData, constantBuffer.GetAddressOf())))
		{
			std::cout << "CreateBuffer() failed. " << std::endl;
			return false;
		}

		return true;
	}
	
	bool createIndexBuffer(const std::vector<uint16_t>& indices, Microsoft::WRL::ComPtr<ID3D11Buffer>& indexBuffer);

	bool createVertexShaderAndInputLayout(const std::wstring& filename,
										  const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElements,
										  Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader,
										  Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout);

	bool createPixelShader(const std::wstring& filename, Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader);

	void updateGui();

	void update(float dt);

	template <typename T_DATA>
	void updateBuffer(const T_DATA& bufferData, Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer)
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		context->Map(buffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
		memcpy(ms.pData, &bufferData, sizeof(bufferData));
		context->Unmap(buffer.Get(), NULL);
	}

	void render();

	float getAspectRatio() const { return float(width) / height; }
};

