#pragma once

#include <windows.h>

#include <vector>
#include <memory>

class CpuRenderer;

enum class ERenderer
{
	None = 0,
	Cpu
};

class Application
{
private:
	constexpr static int windowWidth = 1024;
	constexpr static int windowHeight = 600;

	HWND mainWindowHandle;

	std::unique_ptr<CpuRenderer> renderer;

public:
	Application();
	~Application();

	bool Initialize();
	int Run();
	LRESULT MessageProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	bool initMainWindows();

	bool createRenderer();
};

