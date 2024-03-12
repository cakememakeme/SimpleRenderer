#include "Application.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <directxtk/SimpleMath.h>

#include <iostream>
#include <memory>

#include "CpuRenderer.h"
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

Application* g_app = nullptr;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_app)
    {
        return g_app->MessageProcedure(hWnd, msg, wParam, lParam);
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

Application::Application()
    : mainWindowHandle(nullptr)
{
    g_app = this;
}

Application::~Application()
{
    g_app = nullptr;
}

LRESULT Application::MessageProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    if (!renderer)
    {
        //std::cout << "the renderer does not exist." << std::endl;
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

    switch (msg)
    {
    case WM_SIZE:
    {
        int width = (int)LOWORD(lParam);
        int height = (int)HIWORD(lParam);
        renderer->OnResizeWindow(width, height);
    }
    break;
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

bool Application::Initialize()
{
    if (!initMainWindows())
    {
        return false;
    }

    if (!createRenderer())
    {
        std::cout << "createRenderer() failed." << std::endl;
        return false;
    }

    if (!renderer->Initialize(mainWindowHandle, windowWidth, windowHeight))
    {
        std::cout << "CpuRenderer::Initialize() failed." << std::endl;
        return false;
    }

    renderer->Setup();
    
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
    // wndclass 입력
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

    // 윈도우 실제 크기(해상도) 조정
    RECT windowRect = { 0, 0, windowWidth, windowHeight };
    if (!AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE))
    {
        std::cout << "AdjustWindowRect() failed." << std::endl;
    }

    // 창 생성
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

bool Application::createRenderer()
{
    renderer = std::make_unique<CpuRenderer>();
    return true;
}
