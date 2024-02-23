#pragma once

#include "IRenderer.h"

#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl.h>

enum class ELightType;
class CpuRenderPipeline;
class Object;
class Mesh;
class Light;

struct D3dVertex 
{
	DirectX::SimpleMath::Vector4 pos;
	DirectX::SimpleMath::Vector2 uv;
};

class CpuRenderer : public IRenderer
{
	int width;
	int height;

private:
	// Cpu Rendering
	std::unique_ptr<CpuRenderPipeline> cpuPipeline;

	std::vector<std::shared_ptr<Object>> objects;
	
	std::shared_ptr<Mesh> selectedMesh;

	std::shared_ptr<Light> selectedLight;

	ELightType lightType;

	// ������ �ڵ�
	HWND mainWindowHandle;

	// ����̽�
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	
	// ComPtr ���� ����� �������� ���Ѵ�. ��?
	// ����Ʈ
	D3D11_VIEWPORT viewport;

	// ���� ����Ÿ��
	ID3D11RenderTargetView* renderTargetView;

	// Cpu Rasterization ���� ������ 2DTexture. ���⿡ �ȼ��� �� �������Ѵ�
	ID3D11Texture2D* canvasTexture;
	ID3D11ShaderResourceView* canvasTextureView;

	// �ؽ�ó ������ ���� ���÷�
	ID3D11SamplerState* colorSampler;

	// ���̴�
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layout;

	// ����
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	UINT indexCount;

	// ~~GUI ���~~
	float leftClip = 1.0f;
	float rightClip = 1.0f;
	float topClip = 1.0f;
	float bottomClip = 1.0f;
	float nearClip = 0.0f;

public:
	CpuRenderer();
	virtual ~CpuRenderer();

	bool Create() override;

	bool Initialize(HWND mainWindow, const int bufferWidth, const int bufferHeight) override;

	bool SetObjects(std::vector<std::shared_ptr<Object>>&& receivedObjects) override;

	void Update() override;

	void Render() override;

	void OnPostRender() override;

	bool Reset() override;

private:
	bool initDirect3D();
	bool initShaders();
	bool initGui();

	void updateGui();
	bool selectModel();
	std::vector<DirectX::SimpleMath::Vector4> renderViaCpu();
};

