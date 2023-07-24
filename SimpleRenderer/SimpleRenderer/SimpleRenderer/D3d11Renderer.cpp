#include "D3d11Renderer.h"

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3dcompiler.h>

#include <tuple>
//�������� ���̴� ������
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

	// Geometry ����
	auto [vertices, indices] = GeometryGenerator::MakeBox_TEMP();

	// ���ؽ� ���� �����
	if (!createVertexBuffer(vertices, vertexBuffer))
	{
		cout << "createVertexBuffer() failed." << endl;
		return false;
	}

	// �ε��� ���� �����
	indexCount = UINT(indices.size());

	if (!createIndexBuffer(indices, indexBuffer))
	{
		cout << "createIndexBuffer() failed." << endl;
		return false;
	}

	// ConstantBuffer �����
	constantBufferData.Model = Matrix();
	constantBufferData.View = Matrix();
	constantBufferData.Projection = Matrix();
	if (!createConstantBuffer(constantBufferData, constantBuffer))
	{
		cout << "createConstantBuffer() failed." << endl;
		return false;
	}

	// ���̴� �����
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

	// ��Ÿ ����
	aspect = getAspectRatio();

	return true;
}

bool D3d11Renderer::SetObjects(std::shared_ptr<std::vector<std::shared_ptr<Object>>> receivedObjects)
{
	return false;
}

void D3d11Renderer::Update()
{
	ImGui_ImplDX11_NewFrame(); // GUI ������ ����
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame(); // � �͵��� ������ ���� ��� ����
	ImGui::Begin("Scene Control");

	// ImGui�� �������ִ� Framerate ���
	ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
		ImGui::GetIO().Framerate);

	updateGui(); // �߰������� ����� GUI

	ImGui::End();
	ImGui::Render(); // �������� �͵� ��� ��

	update(ImGui::GetIO().DeltaTime); // �ִϸ��̼� ���� ��ȭ
}

void D3d11Renderer::Render()
{
	render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // GUI ������
	// Switch the back buffer and the front buffer
	// ����: ImGui RenderDrawData() ������ Present() ȣ��
	swapChain->Present(1, 0);
}

void D3d11Renderer::OnPostRender()
{
}

bool D3d11Renderer::Reset()
{
	/*if (renderTargetView)
	{
		renderTargetView->Release();
		renderTargetView = nullptr;
	}

	if (rasterizerSate)
	{
		rasterizerSate->Release();
		rasterizerSate = nullptr;
	}

	if (depthStencilBuffer)
	{
		depthStencilBuffer->Release();
		depthStencilBuffer = nullptr;
	}

	if (depthStencilView)
	{
		depthStencilView->Release();
		depthStencilView = nullptr;
	}

	if (depthStencilState)
	{
		depthStencilState->Release();
		depthStencilState = nullptr;
	}*/

	return true;
}

bool D3d11Renderer::initDirect3D()
{
	// device, context ����
	const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create device
	{
		if (FAILED(D3D11CreateDevice(
			nullptr, // ���� �����, DXGI : directx graphics infrastructer, ���� d3d�� ���������� ����� �� �ְ� ���÷��� ������ ��� ����� ��
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
	UINT msaaQualityLevels = 0;
	{
		device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaaQualityLevels);
		{
			if (msaaQualityLevels <= 0)
			{
				cout << "MSAA not supported." << endl;
			}
		}
	}

	// swap chain
	// ������ �����͸� �Դٰ��� �Ѵ�: Page Flipping
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	{
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// use 32-bit color
		swapChainDesc.BufferCount = 2;									// Double-buffering
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;			// �ֻ���, ����
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;			// �ֻ���, �и� ( 60 / 1 )
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// how swap chain is to be used
		swapChainDesc.OutputWindow = mainWindowHandle;                 // the window to be used
		swapChainDesc.Windowed = TRUE;									// windowed/full-screen mode
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// allow full-screen switching
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		if (msaaQualityLevels > 0)
		{
			swapChainDesc.SampleDesc.Count = 4; // how many multisamples
			swapChainDesc.SampleDesc.Quality = msaaQualityLevels - 1;
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

	// CreateRenderTarget
	{
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
	}

	// Set the viewport
	{
		ZeroMemory(&screenViewport, sizeof(D3D11_VIEWPORT));
		screenViewport.TopLeftX = 0;
		screenViewport.TopLeftY = 0;
		screenViewport.Width = float(width);
		screenViewport.Height = float(height);
		// screenViewport.Width = static_cast<float>(height);
		screenViewport.MinDepth = 0.0f;
		screenViewport.MaxDepth = 1.0f; // Note: important for depth buffering
		context->RSSetViewports(1, &screenViewport);
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
	{
		D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
		depthStencilBufferDesc.Width = width;
		depthStencilBufferDesc.Height = height;
		depthStencilBufferDesc.MipLevels = 1;
		depthStencilBufferDesc.ArraySize = 1;
		depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		if (msaaQualityLevels > 0)
		{
			depthStencilBufferDesc.SampleDesc.Count = 4; // how many multisamples
			depthStencilBufferDesc.SampleDesc.Quality = msaaQualityLevels - 1;
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
		}
		if (FAILED(device->CreateDepthStencilView(depthStencilBuffer.Get(), 0, depthStencilView.GetAddressOf())))
		{
			cout << "CreateDepthStencilView() failed." << endl;
		}
	}

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
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // �ʱ�ȭ �� ����X
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

	// ����: ���̴��� �������� �̸��� "main"�� �Լ��� ����
	HRESULT hr = D3DCompileFromFile(filename.c_str(), 0, 0, "main", "vs_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

	if (FAILED(hr))
	{
		// ������ ���� ���
		if ((hr & D3D11_ERROR_FILE_NOT_FOUND) != 0)
		{
			cout << "File not found." << endl;
		}

		// ���� �޽����� ������ ���
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

	// input layout: vertex shader�� � �����͸� �־����� ���
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
	// ����: �������� ������ �ϱ�
	// "fxc.exe"�� ��ġ�� ���� �ٸ� ���� �ֽ��ϴ�.
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

	// ����: ���̴��� �������� �̸��� "main"�� �Լ��� ����
	HRESULT hr = D3DCompileFromFile(filename.c_str(), 0, 0, "main", "ps_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

	if (FAILED(hr))
	{
		// ������ ���� ���
		if ((hr & D3D11_ERROR_FILE_NOT_FOUND) != 0)
		{
			cout << "File not found." << endl;
		}

		// ���� �޽����� ������ ���
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
	ImGui::SliderFloat("aspect", &aspect, 1.0f, 3.0f);
}

void D3d11Renderer::update(float dt)
{
	// ���� ��ȯ
	constantBufferData.Model = Matrix::CreateScale(modelScaling) *
							   Matrix::CreateRotationY(modelRotation.y) * 
							   Matrix::CreateRotationX(modelRotation.x) *
							   Matrix::CreateRotationZ(modelRotation.z) *
							   Matrix::CreateTranslation(modelTranslation);
	// (����)HLSL �� ������ ��ǥ��, Direct3D�� �޼� ��ǥ��(Row-major)
	constantBufferData.Model = constantBufferData.Model.Transpose(); // R-major���� C-major��

	// ���� ��ȯ
	//constantBufferData.View = XMMatrixLookAtLH(viewEye, viewFocus, viewUp); ���������� XMMatrixLookToLH() ȣ����
	constantBufferData.View = XMMatrixLookToLH(viewEyePos, viewEyeDir, viewUp);
	constantBufferData.View = constantBufferData.View.Transpose();

	// ��������
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
	// IA: Input-Assembler stage
	// VS: Vertex Shader
	// PS: Pixel Shader
	// RS: Rasterizer stage
	// OM: Output-Merger stage // �����ִ� Fragment�� ��� � fragment�� ����� pixel�� ������ ���ΰ�

	context->RSSetViewports(1, &screenViewport);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(renderTargetView.Get(), clearColor);
	context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ��: Depth Buffer�� ������� �ʴ� ���
	// &renderTargetView�� �״�� ���� ������ �����... �Ф� ���̷��°ž� ��ü
	//context->OMSetRenderTargets(1, &renderTargetView, nullptr);
	context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());

	context->OMSetDepthStencilState(depthStencilState.Get(), 0);

	// � ���̴��� ������� ����
	context->VSSetShader(colorVertexShader.Get(), 0, 0);

	/* ��쿡 ���󼭴� �������� �迭�� �־��� ���� �ֽ��ϴ�.
	ID3D11Buffer *pptr[1] = {
		constantBuffer.Get(),
	};
	context->VSSetConstantBuffers(0, 1, pptr); */

	context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
	context->PSSetShader(colorPixelShader.Get(), 0, 0);

	context->RSSetState(rasterizerState.Get());

	// ���ؽ�/�ε��� ���� ����
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetInputLayout(colorInputLayout.Get());
	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->DrawIndexed(indexCount, 0, 0);
}

