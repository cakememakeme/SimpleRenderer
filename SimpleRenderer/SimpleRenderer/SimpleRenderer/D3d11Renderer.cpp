#include "D3d11Renderer.h"

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3dcompiler.h>

#include <tuple>
//수동으로 쉐이더 컴파일
//#include <fstream>
//#include <iterator>
//#include <vector>

#include "Vertex.h"
#include "GeometryGenerator.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;
using namespace std;

D3d11Renderer::D3d11Renderer()
{
}

D3d11Renderer::~D3d11Renderer()
{
	Reset();
}

bool D3d11Renderer::Create()
{
	return false;
}

bool D3d11Renderer::Initialize(HWND mainWindow, const int bufferWidth, const int bufferHeight)
{
	mainWindowHandle = mainWindow;
	width = bufferWidth;
	height = bufferHeight;

	if (!initDirect3D())
	{
		return false;
	}

	if (!initGui())
	{
		return false;
	}

	// Geometry 정의
	auto [vertices, indices] = GeometryGenerator::MakeBox_TEMP();

	// 버텍스 버퍼 만들기
	if (!createVertexBuffer(vertices, vertexBuffer))
	{
		cout << "createVertexBuffer() failed." << endl;
		return false;
	}

	// 인덱스 버퍼 만들기
	indexCount = UINT(indices.size());

	if (!createIndexBuffer(indices, indexBuffer))
	{
		cout << "createIndexBuffer() failed." << endl;
		return false;
	}

	// ConstantBuffer 만들기
	constantBufferData.Model = Matrix();
	constantBufferData.View = Matrix();
	constantBufferData.Projection = Matrix();
	if (!createConstantBuffer(constantBufferData, constantBuffer))
	{
		cout << "createConstantBuffer() failed." << endl;
		return false;
	}

	// 쉐이더 만들기
	// Input-layout objects describe how vertex buffer data is streamed into the
	// IA(Input-Assembler) pipeline stage.
	// https://learn.Microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-iasetinputlayout

	// Input-Assembler Stage
	// The purpose of the input-assembler stage is to read primitive data
	// (points, lines and/or triangles) from user-filled buffers and assemble
	// the data into primitives that will be used by the other pipeline stages.
	// https://learn.Microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-input-assembler-stage

	vector<D3D11_INPUT_ELEMENT_DESC> inputElements = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	if (!createVertexShaderAndInputLayout(L"ColorVertexShader.hlsl", inputElements, colorVertexShader, colorInputLayout))
	{
		cout << "createVertexShaderAndInputLayout() failed." << endl;
		return false;
	}

	if (!createPixelShader(L"ColorPixelShader.hlsl", colorPixelShader))
	{
		cout << "createPixelShader() failed." << endl;
		return false;
	}

	return true;
}

void D3d11Renderer::OnResizeWindow(const int newWidth, const int newHeight)
{
	if (!swapChain)
	{
		cout << "Swapchain missing." << endl;
		return;
	}

	if (!renderTargetView)
	{
		cout << "Rendertargetview missing." << endl;
		return;
	}

	width = newWidth;
	height = newHeight;
	setGuiWidth(0);
	renderTargetView.Reset();
	swapChain->ResizeBuffers(0, 
		static_cast<UINT>(width),
		static_cast<UINT>(height),
		DXGI_FORMAT_UNKNOWN,
		0);
	createRenderTargetView();
	createDepthBuffer();
	setViewport();
}

bool D3d11Renderer::SetObjects(std::vector<std::shared_ptr<Object>>&& receivedObjects)
{
	return false;
}

void D3d11Renderer::Update()
{
	ImGui_ImplDX11_NewFrame(); // GUI 프레임 시작
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame(); // 어떤 것들을 렌더링 할지 기록 시작
	ImGui::Begin("Scene Control");

	// ImGui가 측정해주는 Framerate 출력
	ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
		ImGui::GetIO().Framerate);

	updateGui(); // 추가적으로 사용할 GUI

	ImGui::End();
	ImGui::Render(); // 렌더링할 것들 기록 끝

	update(ImGui::GetIO().DeltaTime); // 애니메이션 같은 변화
}

void D3d11Renderer::Render()
{
	render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // GUI 렌더링
	// Switch the back buffer and the front buffer
	// 주의: ImGui RenderDrawData() 다음에 Present() 호출
	swapChain->Present(1, 0);
}

void D3d11Renderer::OnPostRender()
{
}

bool D3d11Renderer::Reset()
{
	return true;
}

bool D3d11Renderer::initDirect3D()
{
	// device, context 생성
	const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create device
	{
		if (FAILED(D3D11CreateDevice(
			nullptr, // 비디오 어댑터, DXGI : directx graphics infrastructer, 여러 d3d가 공통적으로 사용할 수 있게 디스플레이 저수준 제어를 묶어둔 것
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			&featureLevel,
			1,
			D3D11_SDK_VERSION,
			device.GetAddressOf(),
			nullptr,
			context.GetAddressOf())))
		{
			cout << "D3D11CreateDevice() failed." << endl;
			return false;
		}
		if (featureLevel != D3D_FEATURE_LEVEL_11_0)
		{
			cout << "D3D FEATURE LEVEL 11 0 unsupported." << endl;
			return false;
		}
		if (!device)
		{
			cout << "device failed." << endl;
			return false;
		}
		if (!context)
		{
			cout << "context failed." << endl;
			return false;
		}
	}

	// MSAA
	{
		device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &massSamplingCount);
		{
			if (massSamplingCount <= 0)
			{
				cout << "MSAA not supported." << endl;
			}
		}
	}

	if (!createSwapChain())
	{
		cout << "createSwapChain() failed." << endl;
		return false;
	}

	if (!createRenderTargetView())
	{
		cout << "createRenderTargetView() failed." << endl;
		return false;
	}

	// Set viewport
	{
		bViewportNeedUpdate = true;
		setViewport();
	}

	// Create a rasterizer state
	{
		D3D11_RASTERIZER_DESC rastDesc;
		ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC)); // Need this
		rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
		// rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
		rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
		rastDesc.FrontCounterClockwise = false;
		rastDesc.DepthClipEnable = true;

		device->CreateRasterizerState(&rastDesc, rasterizerState.GetAddressOf());
	}

	// Create depth buffer
	createDepthBuffer();

	// Create depth stencil state
	{
		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
		ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		depthStencilDesc.DepthEnable = true; // false
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
		if (FAILED(device->CreateDepthStencilState(&depthStencilDesc, depthStencilState.GetAddressOf())))
		{
			cout << "CreateDepthStencilState() failed." << endl;
		}
	}

	return true;
}

bool D3d11Renderer::initGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(float(width), float(height));
	ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	if (!ImGui_ImplDX11_Init(device.Get(), context.Get()))
	{
		return false;
	}

	if (!ImGui_ImplWin32_Init(mainWindowHandle))
	{
		return false;
	}

	return true;
}

bool D3d11Renderer::createIndexBuffer(const vector<uint16_t>& indices, ComPtr<ID3D11Buffer>& indexBuffer)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 초기화 후 변경X
	bufferDesc.ByteWidth = UINT(sizeof(uint16_t) * indices.size());
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0; // 0 if no CPU access is necessary.
	bufferDesc.StructureByteStride = sizeof(uint16_t);

	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	indexBufferData.pSysMem = indices.data();
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;

	if (FAILED(device->CreateBuffer(&bufferDesc, &indexBufferData, indexBuffer.GetAddressOf())))
	{
		cout << "CreateBuffer() failed." << endl;
		return false;
	}

	return true;
}

bool D3d11Renderer::createVertexShaderAndInputLayout(
	const wstring& filename, const vector<D3D11_INPUT_ELEMENT_DESC>& inputElements,
	ComPtr<ID3D11VertexShader>& vertexShader, ComPtr<ID3D11InputLayout>& inputLayout)
{
	ComPtr<ID3DBlob> shaderBlob;
	ComPtr<ID3DBlob> errorBlob;

	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// 주의: 쉐이더의 시작점의 이름이 "main"인 함수로 지정
	HRESULT hr = D3DCompileFromFile(filename.c_str(), 0, 0, "main", "vs_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

	if (FAILED(hr))
	{
		// 파일이 없을 경우
		if ((hr & D3D11_ERROR_FILE_NOT_FOUND) != 0)
		{
			cout << "File not found." << endl;
		}

		// 에러 메시지가 있으면 출력
		if (errorBlob)
		{
			cout << "Shader compile error\n" << (char*)errorBlob->GetBufferPointer() << endl;
		}

		return false;
	}

	if (FAILED(device->CreateVertexShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		&vertexShader)))
	{
		cout << "CreateVertexShader() failed.\n" << endl;
		return false;
	}

	// input layout: vertex shader에 어떤 데이터를 넣어줄지 기록
	if (FAILED(device->CreateInputLayout(
		inputElements.data(),
		UINT(inputElements.size()),
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		inputLayout.GetAddressOf())))
	{
		cout << "CreateInputLayout() failed.\n" << endl;
		return false;
	}
	// 참고: 수동으로 컴파일 하기
	// "fxc.exe"의 위치는 각자 다를 수도 있습니다.
	//"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\fxc.exe"
	// C:\Users\jmhong\HongLabGraphicsPart2\06_GraphicsPipeline_Step4_Shaders\ColorVertexShader.hlsl
	// /T "vs_5_0" /E "main" /Fo "ColorVertexShader.cso" /Fx
	// "ColorVertexShader.asm"

	// ifstream input("ColorVertexShader.cso", ios::binary);
	// vector<unsigned char> buffer(istreambuf_iterator<char>(input), {});
	// m_device->CreateVertexShader(buffer.data(), buffer.size(), NULL,
	//                              &vertexShader);

	// m_device->CreateInputLayout(inputElements.data(),
	//                             UINT(inputElements.size()), buffer.data(),
	//                             buffer.size(), &inputLayout);

	return true;
}

bool D3d11Renderer::createPixelShader(const wstring& filename, ComPtr<ID3D11PixelShader>& pixelShader)
{
	ComPtr<ID3DBlob> shaderBlob;
	ComPtr<ID3DBlob> errorBlob;

	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// 주의: 쉐이더의 시작점의 이름이 "main"인 함수로 지정
	HRESULT hr = D3DCompileFromFile(filename.c_str(), 0, 0, "main", "ps_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

	if (FAILED(hr))
	{
		// 파일이 없을 경우
		if ((hr & D3D11_ERROR_FILE_NOT_FOUND) != 0)
		{
			cout << "File not found." << endl;
		}

		// 에러 메시지가 있으면 출력
		if (errorBlob)
		{
			cout << "Shader compile error\n" << (char*)errorBlob->GetBufferPointer() << endl;
		}

		return false;
	}

	if (FAILED(device->CreatePixelShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		nullptr,
		pixelShader.GetAddressOf())))
	{
		cout << "CreatePixelShader() failed.\n" << endl;
		return false;
	}

	return true;
}

bool D3d11Renderer::createSwapChain()
{
	// swap chain
	// 버퍼의 포인터만 왔다갔다 한다: Page Flipping
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	{
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// use 32-bit color
		swapChainDesc.BufferCount = 2;									// Double-buffering
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;			// 주사율, 분자
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;			// 주사율, 분모 ( 60 / 1 )
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// how swap chain is to be used
		swapChainDesc.OutputWindow = mainWindowHandle;                  // the window to be used
		swapChainDesc.Windowed = TRUE;									// windowed/full-screen mode
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// allow full-screen switching
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		if (massSamplingCount > 0)
		{
			swapChainDesc.SampleDesc.Count = 4; // how many multisamples
			swapChainDesc.SampleDesc.Quality = massSamplingCount - 1;
		}
		else
		{
			swapChainDesc.SampleDesc.Count = 1; // how many multisamples
			swapChainDesc.SampleDesc.Quality = 0;
		}
	}

	// Create swap chain
	{
		ComPtr<IDXGIDevice> dxgiDevice;
		device.As(&dxgiDevice);

		ComPtr<IDXGIAdapter> dxgiAdapter;
		dxgiDevice->GetAdapter(&dxgiAdapter);

		ComPtr<IDXGIFactory1> dxgiFactory;
		dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

		if (!dxgiFactory)
		{
			std::cout << "Failed to retrieve the IDXGIFactory1 interface associated with D3D11 device" << endl;
			return false;
		}

		if (FAILED(dxgiFactory->CreateSwapChain(device.Get(), &swapChainDesc, swapChain.GetAddressOf())))
		{
			std::cout << "CreateSwapChain() failed." << endl;
			return false;
		}
	}

	return true;
}

bool D3d11Renderer::createRenderTargetView()
{
	if (!swapChain)
	{
		cout << "Swapchain missing." << endl;
		return false;
	}

	ComPtr<ID3D11Texture2D> backBuffer;
	swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
	if (backBuffer)
	{
		if (FAILED(device->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView.GetAddressOf())))
		{
			cout << "CreateRenderTargetView() failed." << endl;
			return false;
		}
	}
	else
	{
		cout << "Backbuffer missing." << endl;
		return false;
	}

	return true;
}

bool D3d11Renderer::createDepthBuffer()
{
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	depthStencilBufferDesc.Width = width;
	depthStencilBufferDesc.Height = height;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	if (massSamplingCount > 0)
	{
		depthStencilBufferDesc.SampleDesc.Count = 4; // how many multisamples
		depthStencilBufferDesc.SampleDesc.Quality = massSamplingCount - 1;
	}
	else
	{
		depthStencilBufferDesc.SampleDesc.Count = 1; // how many multisamples
		depthStencilBufferDesc.SampleDesc.Quality = 0;
	}
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0;
	depthStencilBufferDesc.MiscFlags = 0;

	if (FAILED(device->CreateTexture2D(&depthStencilBufferDesc, 0, depthStencilBuffer.GetAddressOf())))
	{
		cout << "CreateTexture2D() failed." << endl;
		return false;
	}
	if (FAILED(device->CreateDepthStencilView(depthStencilBuffer.Get(), 0, depthStencilView.GetAddressOf())))
	{
		cout << "CreateDepthStencilView() failed." << endl;
		return false;
	}

	return true;
}

void D3d11Renderer::setViewport()
{
	if (!context)
	{
		cout << "context missing." << endl;
		return;
	}

	if (!bViewportNeedUpdate)
	{
		return;
	}

	bViewportNeedUpdate = false;

	ZeroMemory(&screenViewport, sizeof(D3D11_VIEWPORT));
	screenViewport.TopLeftX = static_cast<float>(guiWidth);
	screenViewport.TopLeftY = 0;
	screenViewport.Width = static_cast<float>(width - guiWidth);
	screenViewport.Height = static_cast<float>(height);
	screenViewport.MinDepth = 0.0f;
	screenViewport.MaxDepth = 1.0f; // Note: important for depth buffering
	context->RSSetViewports(1, &screenViewport);
}

void D3d11Renderer::setGuiWidth(const int newWidth)
{
	if (guiWidth != newWidth)
	{
		bViewportNeedUpdate = true;
		guiWidth = newWidth;
	}
}

void D3d11Renderer::updateGui()
{
	ImGui::Checkbox("usePerspectiveProjection", &bUsePerspectiveProjection);

	ImGui::SliderFloat3("modelTranslation", &modelTranslation.x, -2.0f, 2.0f);
	ImGui::SliderFloat3("modelRotation(Rad)", &modelRotation.x, -3.14f, 3.14f);
	ImGui::SliderFloat3("modelScaling", &modelScaling.x, 0.1f, 2.0f);

	ImGui::SliderFloat3("viewEyePos", &viewEyePos.x, -4.0f, 4.0f);
	ImGui::SliderFloat3("viewEyeDir", &viewEyeDir.x, -4.0f, 4.0f);
	ImGui::SliderFloat3("viewUp", &viewUp.x, -2.0f, 2.0f);
	if (XMVector3Equal(viewUp, XMVectorZero()))
	{
		viewUp.y = 0.001f;
	}

	ImGui::SliderFloat("projFovAngleY(Deg)", &projFovAngleY, 10.0f, 180.0f);
	ImGui::SliderFloat("nearZ", &nearZ, 0.01f, 10.0f);
	ImGui::SliderFloat("farZ", &farZ, nearZ + 0.01f, 15.0f);
	
	ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
	int curGuiWidth = static_cast<int>(ImGui::GetWindowWidth());
	setGuiWidth(curGuiWidth);
}

void D3d11Renderer::update(float dt)
{
	// 모델의 변환
	constantBufferData.Model = Matrix::CreateScale(modelScaling) *
							   Matrix::CreateRotationY(modelRotation.y) * 
							   Matrix::CreateRotationX(modelRotation.x) *
							   Matrix::CreateRotationZ(modelRotation.z) *
							   Matrix::CreateTranslation(modelTranslation);
	// (복습)HLSL 은 오른손 좌표계, Direct3D는 왼손 좌표계(Row-major)
	constantBufferData.Model = constantBufferData.Model.Transpose(); // R-major에서 C-major로

	// 시점 변환
	//constantBufferData.View = XMMatrixLookAtLH(viewEye, viewFocus, viewUp); 내부적으로 XMMatrixLookToLH() 호출함
	constantBufferData.View = XMMatrixLookToLH(viewEyePos, viewEyeDir, viewUp);
	constantBufferData.View = constantBufferData.View.Transpose();

	// 프로젝션
	const float aspect = getAspectRatio();
	if (bUsePerspectiveProjection) 
	{
		constantBufferData.Projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(projFovAngleY), aspect, nearZ, farZ);
	}
	else 
	{
		constantBufferData.Projection = XMMatrixOrthographicOffCenterLH(-aspect, aspect, -1.0f, 1.0f, nearZ, farZ);
	}
	constantBufferData.Projection = constantBufferData.Projection.Transpose();

	updateBuffer(constantBufferData, constantBuffer);
}

void D3d11Renderer::render()
{
	setViewport();

	// IA: Input-Assembler stage
	// VS: Vertex Shader
	// PS: Pixel Shader
	// RS: Rasterizer stage
	// OM: Output-Merger stage // 겹쳐있는 Fragment의 경우 어떤 fragment를 출력할 pixel로 선택할 것인가

	context->RSSetViewports(1, &screenViewport);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(renderTargetView.Get(), clearColor);
	context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 비교: Depth Buffer를 사용하지 않는 경우
	// &renderTargetView를 그대로 쓰면 문제가 생긴다... ㅠㅠ 왜이러는거야 대체
	//context->OMSetRenderTargets(1, &renderTargetView, nullptr);
	context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());

	context->OMSetDepthStencilState(depthStencilState.Get(), 0);

	// 어떤 쉐이더를 사용할지 설정
	context->VSSetShader(colorVertexShader.Get(), 0, 0);

	/* 경우에 따라서는 포인터의 배열을 넣어줄 수도 있습니다.
	ID3D11Buffer *pptr[1] = {
		constantBuffer.Get(),
	};
	context->VSSetConstantBuffers(0, 1, pptr); */

	context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
	context->PSSetShader(colorPixelShader.Get(), 0, 0);

	context->RSSetState(rasterizerState.Get());

	// 버텍스/인덱스 버퍼 설정
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetInputLayout(colorInputLayout.Get());
	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->DrawIndexed(indexCount, 0, 0);
}

