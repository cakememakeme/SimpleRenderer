#pragma once

#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl.h>

#include <memory>

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

// based on d3d11
class CpuRenderer
{
private:
	int width;
	int height;

	// Cpu Rendering
	std::unique_ptr<CpuRenderPipeline> pipeline;
	std::vector<std::shared_ptr<Object>> objects;

	// 윈도우 핸들
	HWND mainWindowHandle;

	// 디바이스
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	// 뷰포트
	D3D11_VIEWPORT viewport;

	// 메인 렌더타겟
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;

	// Cpu Rasterization 구현 목적의 2DTexture. 여기에 픽셀을 찍어서 렌더링한다
	Microsoft::WRL::ComPtr<ID3D11Texture2D> canvasTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> canvasTextureView;

	// 텍스처 설정을 위한 샘플러
	Microsoft::WRL::ComPtr<ID3D11SamplerState> colorSampler;

	// 쉐이더
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> layout;

	// 버퍼
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	UINT indexCount;

	// ~~GUI 기능~~
	float guiWidth = 0.0f;
	float leftClip = 1.0f;
	float rightClip = 1.0f;
	float topClip = 1.0f;
	float bottomClip = 1.0f;
	float nearClip = 0.0f;
	bool bViewportNeedUpdate = false;

	// 피킹 구현 전 임시 변수
	std::weak_ptr<Mesh> selectedMesh;
	std::weak_ptr<Light> selectedLight;

	// 현재 라이트 타입
	ELightType lightType;

public:
	CpuRenderer();
	virtual ~CpuRenderer();

	virtual bool Create();

	virtual bool Initialize(HWND mainWindow, const int bufferWidth, const int bufferHeight);

	virtual bool Setup();

	virtual void OnResizeWindow(const int inWidth, const int inHeight);

	virtual void Update();

	virtual void Render();

	virtual void OnPostRender();

	virtual bool Reset();

private:
	bool initDirect3D();
	bool initShaders();
	bool initGui();
	bool createRenderTargetView();
	bool createSwapChain();
	bool createCanvasTexture();
	std::vector<std::shared_ptr<Object>> generateObjects();

	void setViewport();
	void updateGui();
	void setGuiWidth(const int inWidth);
	bool selectModel();
	std::vector<DirectX::SimpleMath::Vector4> cpuRender();
};

