#include "CpuRenderer.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <iostream>

#include "CpuRenderPipeline.h"
#include "Mesh.h"
#include "Light.h"
#include "GeometryGenerator.h"

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;
using namespace std;

CpuRenderer::CpuRenderer()
{
	Create();
}

CpuRenderer::~CpuRenderer()
{
	Reset();
}

bool CpuRenderer::Create()
{
	pipeline = make_unique<CpuRenderPipeline>();
	if (!pipeline)
	{
		return false;
	}

	return true;
}

bool CpuRenderer::Initialize(HWND mainWindow, const int bufferWidth, const int bufferHeight)
{
	mainWindowHandle = mainWindow;
	width = bufferWidth;
	height = bufferHeight;

	if (!initDirect3D())
	{
		return false;
	}

	if (!pipeline->Initialize(bufferWidth, bufferHeight))
	{
		return false;
	}

	if (!initGui())
	{
		return false;
	}

	return true;
}

bool CpuRenderer::Setup()
{
	objects = generateObjects();

	if (objects.empty())
	{
		return false;
	}
	
	return true;
}

void CpuRenderer::OnResizeWindow(const int inWidth, const int inHeight)
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

	width = inWidth;
	height = inHeight;
	renderTargetView.Reset();
	swapChain->ResizeBuffers(0,
		static_cast<UINT>(width),
		static_cast<UINT>(height),
		DXGI_FORMAT_UNKNOWN,
		0);
	createRenderTargetView();
	createCanvasTexture();
	pipeline->Initialize(width, height);
	setGuiWidth(0);
	setViewport();
}

void CpuRenderer::Update()
{
	if (!selectModel())
	{
		//std::cout << "no model selecedted." << std::endl;
	}

	updateGui();

	pipeline->SetObjects(objects);
	pipeline->SetLightType(lightType);
	pipeline->SetClippingPlane(leftClip, rightClip, topClip, bottomClip, nearClip);
}

void CpuRenderer::Render()
{
	// copy to resource
	{
		vector<Vector4> displayBuffer = cpuRender();
		if (displayBuffer.empty())
		{
			std::cout << "cpuRender() failed." << std::endl;
			displayBuffer.resize(width * height);
			std::fill(displayBuffer.begin(), displayBuffer.end(), Vector4{ 0.0f, 0.0f, 0.0f, 1.0f });
		}
		D3D11_MAPPED_SUBRESOURCE subresource = {};
		context->Map(canvasTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
		// 메모리 패딩 고려
		// memcpy(subresource.pData, displayBuffer.data(), displayBuffer.size() * sizeof(Vector4));
		{
			char* pData = (char*)subresource.pData;
			char* data = (char*)displayBuffer.data();
			size_t stride = width * sizeof(Vector4);
			for (int i = 0; i < height; i++) 
			{
				memcpy(pData, data, stride);
				pData += subresource.RowPitch;
				data += stride;
			}
		}
		context->Unmap(canvasTexture.Get(), 0);
	}

	// 디바이스에 렌더링
	{
		setViewport();
		context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);
		float clearColor[4] = { 0.0f, 0.2f, 0.0f, 1.0f };
		context->ClearRenderTargetView(renderTargetView.Get(), clearColor);

		context->VSSetShader(vertexShader.Get(), 0, 0);
		context->PSSetShader(pixelShader.Get(), 0, 0);

		UINT stride = sizeof(D3dVertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	
		context->PSSetSamplers(0, 1, colorSampler.GetAddressOf());
		context->PSSetShaderResources(0, 1, canvasTextureView.GetAddressOf());
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->DrawIndexed(indexCount, 0, 0);
	}
}

void CpuRenderer::OnPostRender()
{
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	swapChain->Present(1, 0);
}

bool CpuRenderer::Reset()
{
	DestroyWindow(mainWindowHandle);
	return false;
}

bool CpuRenderer::initDirect3D()
{
	//https://learn.microsoft.com/ko-kr/windows/uwp/gaming/simple-port-from-direct3d-9-to-11-1-part-1--initializing-direct3d

	// device, context 생성
	const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
	UINT createDeviceFlags = 0;
	const D3D_FEATURE_LEVEL featureLevels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_9_3 };
	D3D_FEATURE_LEVEL featureLevel;
	{
		if (FAILED(D3D11CreateDevice(nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			0,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&device,
			&featureLevel,
			&context)))
		{
			std::cout << "D3D11CreateDevice() failed." << std::endl;
			return false;
		}
		if (featureLevel != D3D_FEATURE_LEVEL_11_0)
		{
			std::cout << "D3D FEATURE LEVEL 11 0 unsupported." << std::endl;
			return false;
		}
	}

	// 스왑체인
	if(!createSwapChain())
	{
		cout << "createSwapChain() failed." << endl;
		return false;
	}

	// 프레임버퍼
	if(!createRenderTargetView())
	{
		cout << "createRenderTargetView() failed." << endl;
		return false;
	}

	// 뷰포트
	bViewportNeedUpdate = true;
	setViewport();

	if (!initShaders())
	{
		std::cout << "initShaders() failed." << std::endl;
		return false;
	}

	// 텍스처 샘플러 설정
	D3D11_SAMPLER_DESC sampDesc = {};
	{
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; // D3D11_FILTER_MIN_MAG_MIP_LINEAR
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		if (FAILED(device->CreateSamplerState(&sampDesc, colorSampler.GetAddressOf())))
		{
			std::cout << "CreateSamplerState() failed." << std::endl;
			return false;
		}
	}

	// plane을 화면에 깔고, 거기에 CpuRenderPipeline에서 처리한 fragment를 plane에 그린다
	// plane 메시데이터 설정
	{
		const std::vector<D3dVertex> vertices = {
			{
				{-1.0f, -1.0f, 0.0f, 1.0f},
				{0.f, 1.f},
			},
			{
				{1.0f, -1.0f, 0.0f, 1.0f},
				{1.f, 1.f},
			},
			{
				{1.0f, 1.0f, 0.0f, 1.0f},
				{1.f, 0.f},
			},
			{
				{-1.0f, 1.0f, 0.0f, 1.0f},
				{0.f, 0.f},
			},
		};

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // write access access by
		// CPU and GPU
		bufferDesc.ByteWidth = UINT(sizeof(D3dVertex) * vertices.size()); // size is the VERTEX struct * 3
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // use as a vertex buffer
		bufferDesc.CPUAccessFlags =	D3D11_CPU_ACCESS_WRITE; // allow CPU to write
		// in buffer
		bufferDesc.StructureByteStride = sizeof(D3dVertex);

		D3D11_SUBRESOURCE_DATA vertexBufferData = {	nullptr, 0, 0 };
		vertexBufferData.pSysMem = vertices.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;

		const HRESULT hr = device->CreateBuffer(&bufferDesc, &vertexBufferData, vertexBuffer.GetAddressOf());
		if (FAILED(hr)) 
		{
			std::cout << "CreateBuffer() failed. " << std::hex << hr << std::endl;
		};
	}

	// 인덱스 설정
	{
		const std::vector<uint16_t> indices = {
			3, 1, 0, 2, 1, 3,
		};

		indexCount = UINT(indices.size());

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // write access access by
		// CPU and GPU
		bufferDesc.ByteWidth = UINT(sizeof(uint16_t) * indices.size());
		bufferDesc.BindFlags =
			D3D11_BIND_INDEX_BUFFER; // use as a vertex buffer
		bufferDesc.CPUAccessFlags =
			D3D11_CPU_ACCESS_WRITE; // allow CPU to write
		// in buffer
		bufferDesc.StructureByteStride = sizeof(uint16_t);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = indices.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;

		device->CreateBuffer(&bufferDesc, &indexBufferData, indexBuffer.GetAddressOf());
	}

	// 캔버스 텍스처 설정
	if (!createCanvasTexture())
	{
		std::cout << "createCanvasTexture() failed." << std::endl;
		return false;
	}
	
	return true;
}

bool CpuRenderer::initGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
	
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::StyleColorsLight();

	if (!ImGui_ImplDX11_Init(device.Get(), context.Get()))
	{
		std::cout << "ImGui_ImplDX11_Init() failed." << std::endl;
		return false;
	}

	if (!ImGui_ImplWin32_Init(mainWindowHandle))
	{
		std::cout << "ImGui_ImplWin32_Init() failed." << std::endl;
		return false;
	}

	return true;
}

bool CpuRenderer::createRenderTargetView()
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

bool CpuRenderer::createSwapChain()
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
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
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

bool CpuRenderer::createCanvasTexture()
{
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	textureDesc.ArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DYNAMIC;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.MiscFlags = 0;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	textureDesc.Width = width;
	textureDesc.Height = height;

	if (FAILED(device->CreateTexture2D(&textureDesc, nullptr, canvasTexture.GetAddressOf())))
	{
		std::cout << "CreateTexture2D() failed." << std::endl;
		return false;
	}

	if (FAILED(device->CreateShaderResourceView(canvasTexture.Get(), nullptr, canvasTextureView.GetAddressOf())))
	{
		std::cout << "CreateShaderResourceView() failed." << std::endl;
		return false;
	}
	return true;
}

std::vector<std::shared_ptr<Object>> CpuRenderer::generateObjects()
{
	vector<shared_ptr<Object>> objects = std::vector<std::shared_ptr<Object>>();

	// 메시 설정
	{
		std::vector<Mesh> meshes = GeometryGenerator::ReadFromFile("./groza/", "Wp_Gun_Groza.fbx");
		objects.reserve(meshes.size());
		for (const Mesh& mesh : meshes)
		{
			std::shared_ptr<Mesh> newMesh = make_shared<Mesh>(mesh);
			newMesh->Transform.rotationX += 0.15f;
			newMesh->Transform.translation.y -= 0.2f;
			newMesh->Transform.translation.z += 0.2f;
			newMesh->Transform.scale = Vector3(2.0f);
			objects.push_back(newMesh);
		}

		//테스트
		//{
		//	//박스
		//	std::shared_ptr<Mesh> mesh = make_shared<Mesh>(GeometryGenerator::MakeBox());
		//	
		//	//평면
		//	//std::shared_ptr<Mesh> mesh = make_shared<Mesh>(GeometryGenerator::MakePlane());
		//	if (mesh)
		//	{
		//	    constexpr float toRadian = 3.141592f / 180.0f;
		//	    //mesh->TestBox();
		//	    mesh->Transform.translation = Vector3(0.0f, -0.3f, 1.0f);
		//	    mesh->Transform.rotationX = 60.0f * toRadian;
		//	    mesh->Transform.rotationY = 40.0f * toRadian;
		//	    mesh->Transform.rotationZ = -20.0f * toRadian;
		//	    mesh->Transform.scale = Vector3(0.5f, 0.5f, 0.5f);
		//	    objects.push_back(mesh);
		//	}
		//}

		// 스카이 스피어
		{
			//// texassemble.exe cube -w 2048 -h 2048 -o saintpeters.dds posx.jpg negx.jpg
			//// posy.jpg negy.jpg posz.jpg negz.jpg texassemble.exe cube -w 2048 -h 2048
			//// -o skybox.dds right.jpg left.jpg top.jpg bottom.jpg front.jpg back.jpg -y
			//// https://github.com/Microsoft/DirectXTex/wiki/Texassemble

			//// .dds 파일 읽어들여서 초기화
			//Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
			//auto hr = CreateDDSTextureFromFileEx(
			//    this->m_device.Get(), L"./skybox/skybox.dds", 0, D3D11_USAGE_DEFAULT,
			//    D3D11_BIND_SHADER_RESOURCE, 0,
			//    D3D11_RESOURCE_MISC_TEXTURECUBE, // 큐브맵용 텍스춰
			//    DDS_LOADER_FLAGS(false), (ID3D11Resource**)texture.GetAddressOf(),
			//    this->m_cubeMapping.cubemapResourceView.GetAddressOf(), nullptr);

			//if (FAILED(hr)) {
			//    std::cout << "CreateDDSTextureFromFileEx() failed" << std::endl;
			//}

			//m_cubeMapping.cubeMesh = std::make_shared<Mesh>();

			//m_BasicVertexConstantBufferData.model = Matrix();
			//m_BasicVertexConstantBufferData.view = Matrix();
			//m_BasicVertexConstantBufferData.projection = Matrix();
			//ComPtr<ID3D11Buffer> vertexConstantBuffer;
			//ComPtr<ID3D11Buffer> pixelConstantBuffer;
			//AppBase::CreateConstantBuffer(m_BasicVertexConstantBufferData,
			//    m_cubeMapping.cubeMesh->vertexConstantBuffer);
			//AppBase::CreateConstantBuffer(m_BasicPixelConstantBufferData,
			//    m_cubeMapping.cubeMesh->pixelConstantBuffer);

			//// 커다란 박스 초기화
			//// - 세상이 커다란 박스 안에 갇혀 있는 구조입니다.
			//// - D3D11_CULL_MODE::D3D11_CULL_NONE 또는 삼각형 뒤집기
			//// - 예시) std::reverse(myvector.begin(),myvector.end());
			////MeshData cubeMeshData = GeometryGenerator::MakeBox(20.0f);
			//MeshData cubeMeshData = GeometryGenerator::MakeSphere(20.0f, 100, 100);
			//std::reverse(cubeMeshData.indices.begin(), cubeMeshData.indices.end());

			//AppBase::CreateVertexBuffer(cubeMeshData.vertices,
			//    m_cubeMapping.cubeMesh->vertexBuffer);
			//m_cubeMapping.cubeMesh->m_indexCount = UINT(cubeMeshData.indices.size());
			//AppBase::CreateIndexBuffer(cubeMeshData.indices,
			//    m_cubeMapping.cubeMesh->indexBuffer);

			//// 쉐이더 초기화

			//// 다른 쉐이더와 동일한 InputLayout 입니다.
			//// 실제로는 "POSITION"만 사용합니다.
			//vector<D3D11_INPUT_ELEMENT_DESC> basicInputElements = {
			//    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			//        D3D11_INPUT_PER_VERTEX_DATA, 0},
			//    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,
			//        D3D11_INPUT_PER_VERTEX_DATA, 0},
			//    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3,
			//        D3D11_INPUT_PER_VERTEX_DATA, 0},
			//};

			//AppBase::CreateVertexShaderAndInputLayout(
			//    L"CubeMappingVertexShader.hlsl", basicInputElements,
			//    m_cubeMapping.vertexShader, m_cubeMapping.inputLayout);

			//AppBase::CreatePixelShader(L"CubeMappingPixelShader.hlsl",
			//    m_cubeMapping.pixelShader);

			//// 기타
			//// - 텍스춰 샘플러도 다른 텍스춰와 같이 사용
		}
	}

	// 라이트
	{
		std::shared_ptr<Light> light = make_shared<Light>();
		light->Strength = Vector3(1.0f);
		light->Direction = Vector3(0.0f, -0.5f, 0.5f);
		objects.push_back(light);
	}

	return objects;
}

void CpuRenderer::setViewport()
{
	if (!context)
	{
		cout << "context missing." << endl;
		return;
	}

	//if (!bViewportNeedUpdate)
	//{
	//	return;
	//}

	bViewportNeedUpdate = false;

	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	//viewport.TopLeftX = static_cast<float>(guiWidth);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	//viewport.Width = static_cast<float>(width - guiWidth);
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f; // 실제 깊이 테스트는 다른 곳에서 이루어진다
	context->RSSetViewports(1, &viewport);
}

void CpuRenderer::updateGui()
{
	// begin update gui
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Scene Control");
	}

	// gui update
	std::shared_ptr<Mesh> mesh = selectedMesh.lock();
	std::shared_ptr<Light> light = selectedLight.lock();
	if(mesh && light)
	{
		ImGui::SliderFloat("Clipping Left", &leftClip, -1.0f, 1.0f);

		ImGui::SliderFloat("Clipping Right", &rightClip, -1.0f, 1.0f);

		ImGui::SliderFloat("Clipping Top", &topClip, -1.0f, 1.0f);

		ImGui::SliderFloat("Clipping Bottom", &bottomClip, -1.0f, 1.0f);

		ImGui::SliderFloat("Clipping Near", &nearClip, 0.0f, 1.0f);

		ImGui::SliderAngle("Object RotationAboutX", &mesh->Transform.rotationX);

		ImGui::SliderAngle("Object RotationAboutY", &mesh->Transform.rotationY);

		ImGui::SliderAngle("Object RotationAboutZ", &mesh->Transform.rotationZ);

		ImGui::SliderFloat3("Object Translation", &mesh->Transform.translation.x, -3.0f, 4.0f);

		ImGui::SliderFloat3("Object Scale", &mesh->Transform.scale.x, 0.1f, 2.0f);

		ImGui::SliderFloat3("Material ambient", &mesh->Material.Ambient.x, 0.0f, 1.0f);

		if (!mesh->Material.diffuseTex)
		{
			ImGui::SliderFloat3("Material diffuse", &mesh->Material.Diffuse.x, 0.0f, 1.0f);
		}

		ImGui::SliderFloat3("Material specular", &mesh->Material.Specular.x, 0.0f, 1.0f);

		ImGui::SliderFloat("Material shininess", &mesh->Material.Shininess, 0.0f, 256.0f);

		if (ImGui::RadioButton("Directional Light", static_cast<int>(lightType) == 0))
		{
			lightType = ELightType::Directional;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Point Light", static_cast<int>(lightType) == 1))
		{
			lightType = ELightType::Point;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Spot Light", static_cast<int>(lightType) == 2))
		{
			lightType = ELightType::Spot;
		}

		ImGui::SliderFloat3("Light Strength", &light->Strength.x, 0.0f, 1.0f);

		if (ImGui::SliderFloat3("Light Direction", &light->Direction.x, -3.0f, 3.0f))
		{
			if (light->Direction.Length() > 1e-5f)
			{
				light->Direction.Normalize();
			}
		};

		ImGui::SliderFloat3("Light Position", &light->Position.x, -2.0f, 2.0f);

		ImGui::SliderFloat("Light fallOffStart", &light->FallOffStart, 0.0f, 5.0f);

		ImGui::SliderFloat("Light fallOffEnd", &light->FallOffEnd, 0.0f, 10.0f);

		ImGui::SliderFloat("Light spotPower", &light->SpotPower, 0.0f, 512.0f);
	}

	// end update gui
	{
		ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
		int curGuiWidth = static_cast<int>(ImGui::GetWindowWidth());
		setGuiWidth(curGuiWidth);

		ImGui::End();
		ImGui::Render();
	}
}

void CpuRenderer::setGuiWidth(const int inWidth)
{
	if (guiWidth != inWidth)
	{
		bViewportNeedUpdate = true;
		guiWidth = inWidth;
	}
}

vector<Vector4> CpuRenderer::cpuRender()
{
	return pipeline->Process();
}

bool CpuRenderer::selectModel()
{
	if (objects.empty())
	{
		return false;
	}

	// @todo. 여러 개의 오브젝트 설정 구현
	for (auto& object : objects)
	{
		shared_ptr<Mesh> mesh = dynamic_pointer_cast<Mesh>(object);
		if (mesh)
		{
			selectedMesh = mesh;
		}
		shared_ptr<Light> light = dynamic_pointer_cast<Light>(object);
		if (light)
		{
			selectedLight = light;
		}
	}

	if (selectedMesh.lock() && selectedLight.lock())
	{
		return true;
	}

	return false;
}

bool CpuRenderer::initShaders()
{
	ComPtr<ID3DBlob> vertexBlob = nullptr;
	ComPtr<ID3DBlob> pixelBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	if (FAILED(D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexBlob, &errorBlob)))
	{
		if (errorBlob)
		{
			std::cout << "Vertex shader compile errer\n" << (char*)errorBlob->GetBufferPointer() << std::endl;
			return false;
		}
	}

	if (FAILED(D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelBlob, &errorBlob)))
	{
		if (errorBlob)
		{
			std::cout << "Pixel shader compile errer\n" << (char*)errorBlob->GetBufferPointer() << std::endl;
			return false;
		}
	}

	if (FAILED(device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf())))
	{
		std::cout << "CreateVertexShader() failed." << std::endl;
		return false;
	}

	if (FAILED(device->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf())))
	{
		std::cout << "CreatePixelShader() failed." << std::endl;
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC inputElemnetDest[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	if (FAILED(device->CreateInputLayout(inputElemnetDest, 2, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), layout.GetAddressOf())))
	{
		std::cout << "CreateInputLayout() failed." << std::endl;
		return false;
	}

	context->IASetInputLayout(layout.Get());
	return true;
}
