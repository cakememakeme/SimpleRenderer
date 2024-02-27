#include "Application.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <directxtk/SimpleMath.h>

#include <iostream>
#include <memory>

#include "CpuRenderer.h"
#include "D3d11Renderer.h"
#include "Mesh.h"
#include "Light.h"
#include "GeometryGenerator.h"

using namespace DirectX::SimpleMath;
using namespace std;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg) 
    {
    case WM_SIZE:
        // Reset and resize swapchain
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_MOUSEMOVE:
        // std::cout << "Mouse " << LOWORD(lParam) << " " << HIWORD(lParam) << std::endl;
        break;
    case WM_LBUTTONUP:
        // std::cout << "WM_LBUTTONUP Left mouse button" << std::endl;
        break;
    case WM_RBUTTONUP:
        // std::cout << "WM_RBUTTONUP Right mouse button" << std::endl;
        break;
    case WM_KEYDOWN:
        // std::cout << "WM_KEYDOWN " << (int)wParam << std::endl;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

Application::Application()
{

}

Application::~Application()
{
}

bool Application::Initialize(const ERenderer type)
{
    if (!initMainWindows())
    {
        return false;
    }

    if (!createRenderer(type))
    {
        std::cout << "createRenderer() failed." << std::endl;
        return false;
    }

    if (!renderer->Initialize(mainWindowHandle, windowWidth, windowHeight))
    {
        std::cout << "IRenderer::Initialize() failed." << std::endl;
        return false;
    }

    vector<shared_ptr<Object>> objects = std::vector<std::shared_ptr<Object>>();
    
    // �޽� ����
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
    
    //�׽�Ʈ
    {
        ////�ڽ�
        //std::shared_ptr<Mesh> mesh = make_shared<Mesh>(GeometryGenerator::MakeBox());
        //
        ////���
        ////std::shared_ptr<Mesh> mesh = make_shared<Mesh>(GeometryGenerator::MakePlane());
        //if (mesh)
        //{
        //    constexpr float toRadian = 3.141592f / 180.0f;
        //    //mesh->TestBox();
        //    mesh->Transform.translation = Vector3(0.0f, -0.3f, 1.0f);
        //    mesh->Transform.rotationX = 60.0f * toRadian;
        //    mesh->Transform.rotationY = 40.0f * toRadian;
        //    mesh->Transform.rotationZ = -20.0f * toRadian;
        //    mesh->Transform.scale = Vector3(0.5f, 0.5f, 0.5f);
        //    objects.push_back(mesh);
        //}
    }

    std::shared_ptr<Light> light = make_shared<Light>();
    if (light)
    {
        light->Strength = Vector3(1.0f);
        light->Direction = Vector3(0.0f, -0.5f, 0.5f);
        objects.push_back(light);
    }

    renderer->SetObjects(std::move(objects));
    
	return true;
}

int Application::Run()
{
    // Main message loop
    MSG msg = {};
    while (WM_QUIT != msg.message) 
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            renderer->Update();
            renderer->Render();

            renderer->OnPostRender();
        }
    }

	return 0;
}

bool Application::initMainWindows()
{
    // wndclass �Է�
    WNDCLASSEX windowClass = { sizeof(WNDCLASSEX),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(NULL),
        NULL,
        NULL,
        NULL,
        NULL,
        L"CpuRenderer",
        NULL };
    if (!RegisterClassEx(&windowClass))
    {
        std::cout << "RegisterClassEx() failed." << std::endl;
        return false;
    }

    // ������ ���� ũ��(�ػ�) ����
    RECT windowRect = { 0, 0, windowWidth, windowHeight };
    if (!AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE))
    {
        std::cout << "AdjustWindowRect() failed." << std::endl;
    }

    // â ����
    mainWindowHandle = CreateWindow(windowClass.lpszClassName,
        L"Simple CpuRenderer",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL, NULL, windowClass.hInstance, NULL);
    if (!mainWindowHandle)
    {
        std::cout << "CreateWindow() failed." << std::endl;
        return false;
    }

    // show & update
    ShowWindow(mainWindowHandle, SW_SHOWDEFAULT);
    UpdateWindow(mainWindowHandle);

    return true;
}

bool Application::createRenderer(const ERenderer& type)
{
    switch (type)
    {
        case ERenderer::Cpu:
        {
            renderer = std::make_unique<CpuRenderer>();
        }
        break;
        case ERenderer::D3d11:
        {
            renderer = std::make_unique<D3d11Renderer>();
        }
        break;
        default:
        {
            return false;
        }
    }
    return true;
}
