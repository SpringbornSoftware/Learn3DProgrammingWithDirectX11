#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include "GameTimer.h"
#include <string>

#include <cassert>
#include <windowsx.h>

#include <comdef.h>
#include <iostream>


#pragma comment(lib, "d3d11.lib")

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	virtual ~D3DApp();

	HINSTANCE getInstance()const;
	HWND      getWindow()const;
	float     getAspectRatio()const;

	int	run();

	// Framework methods.  Derived client class overrides these methods to 
	// implement specific application requirements.
	virtual bool init();
	virtual void onResized();
	virtual void updateScene(float dt) = 0;
	virtual void drawScene() = 0;
	virtual LRESULT handleWindowsMessage(
		HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Convenience overrides for handling mouse input.
	virtual void onMouseDown(WPARAM btnState, int x, int y) {}
	virtual void onMouseUp(WPARAM btnState, int x, int y) {}
	virtual void onMouseMove(WPARAM btnState, int x, int y) {}

protected:
	bool initMainWindow();
	bool initDirect3D();
	void calculateFrameStats();

protected:
	HINSTANCE appInstance_;
	HWND      mainWindow_;
	bool      appPaused_;
	bool      minimized_;
	bool      maximized_;
	bool      resizing_;
	UINT      msaaQuality_;
	GameTimer timer_;

	// com ptrs for directx
	Microsoft::WRL::ComPtr<ID3D11Device>			device_;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		deviceContext_;
	Microsoft::WRL::ComPtr<IDXGISwapChain>			swapChain_;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>			depthStencilBuffer_;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	renderTargetView_;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	depthStencilView_;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	rasterState_;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState_;
	D3D11_VIEWPORT									screenViewport_;

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring	mainWindowCaption_;
	D3D_DRIVER_TYPE driverType_;
	int				screenWidth_;
	int				screenHeight_;
	bool			enable4xMsaa_;
};



#define HR(x)                                              \
{                                                          \
    HRESULT hr = (x);                                      \
    if (FAILED(hr))                                        \
    {                                                      \
        _com_error err(hr);                                \
        std::wcout << L"File: " << __FILE__ << L"\nLine: " << __LINE__ << L"\nError: " << err.ErrorMessage() << L"\nExpression: " << L#x << std::endl; \
        exit(EXIT_FAILURE);                                \
    }                                                      \
}
