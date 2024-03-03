#pragma once

#include <memory>

#include "IRenderer.h"

enum class ERenderer
{
	None = 0,
	D3d11,
	Cpu,
	//D3d11Rt,
	//D3d11Pbr,
	//Vk,
	//VkRt,
	//VkPbr
};

class Application
{
private:
	constexpr static int windowWidth = 1024;
	constexpr static int windowHeight = 600;

	HWND mainWindowHandle;

	std::unique_ptr<IRenderer> renderer;

public:
	Application();
	~Application();

	bool Initialize(const ERenderer type);
	int Run();
	LRESULT MessageProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	std::vector<std::shared_ptr<Object>> generateObjects();

	bool initMainWindows();

	bool createRenderer(const ERenderer& type);
};

