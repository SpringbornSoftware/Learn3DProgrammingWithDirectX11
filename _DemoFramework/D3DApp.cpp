#include "D3DApp.h"

using Microsoft::WRL::ComPtr;

namespace
{
// This is just used to forward Windows messages from a global window
// procedure to our member function window procedure because we cannot
// assign a member function to WNDCLASS::lpfnWndProc.
D3DApp* gd3dApp = 0;
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return gd3dApp->handleWindowsMessage(hwnd, msg, wParam, lParam);
}

D3DApp::D3DApp(HINSTANCE hInstance)
	:
	appInstance_(hInstance),
	mainWindowCaption_(L"D3D11 Application"),
	driverType_(D3D_DRIVER_TYPE_HARDWARE),
	screenWidth_(800),
	screenHeight_(600),
	mainWindow_(0),
	appPaused_(false),
	minimized_(false),
	maximized_(false),
	resizing_(false),
	msaaQuality_(0),
	timer_(),
	device_(),
	deviceContext_(),
	swapChain_(),
	depthStencilBuffer_(),
	renderTargetView_(),
	depthStencilView_(),
	rasterState_()
{
	gd3dApp = this;
}

D3DApp::~D3DApp()
{
	if (deviceContext_)
	{
		deviceContext_->ClearState();
	}
}

HINSTANCE D3DApp::getInstance() const
{
	return appInstance_;
}

HWND D3DApp::getWindow() const
{
	return mainWindow_;
}

float D3DApp::getAspectRatio() const
{
	return static_cast<float>(screenWidth_) / screenHeight_;
}

int D3DApp::run()
{
	MSG message = {};
	timer_.reset();

	while (message.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			timer_.tick();

			if (!appPaused_)
			{
				calculateFrameStats();
				updateScene(timer_.getDeltaTime());
				drawScene();
			}
			else
			{
				Sleep(100);
			}
		}
	}
	return static_cast<int>(message.wParam);
}

bool D3DApp::init()
{
	if (!initMainWindow())
	{
		return false;
	}
	if (!initDirect3D())
	{
		return false;
	}
	return true;
}

void D3DApp::onResized()
{
	assert(deviceContext_);
	assert(device_);
	assert(swapChain_);

	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.

	renderTargetView_.Reset();
	depthStencilView_.Reset();
	depthStencilBuffer_.Reset();

	// Resize the swap chain and recreate the render target view.

	HR(swapChain_->ResizeBuffers(1, screenWidth_, screenHeight_, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ComPtr<ID3D11Texture2D> backBuffer;
	HR(swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf())));
	HR(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView_.GetAddressOf()));

	// Create the depth/stencil buffer and view.

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = screenWidth_;
	depthStencilDesc.Height = screenHeight_;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if (enable4xMsaa_)
	{
		depthStencilDesc.SampleDesc.Count = 4;
		depthStencilDesc.SampleDesc.Quality = msaaQuality_ - 1;
	}
	// No MSAA
	else
	{
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}

	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	HR(device_->CreateTexture2D(&depthStencilDesc, nullptr, depthStencilBuffer_.GetAddressOf()));
	HR(device_->CreateDepthStencilView(depthStencilBuffer_.Get(), nullptr, depthStencilView_.GetAddressOf()));

	// Bind the render target view and depth/stencil view to the pipeline.

	deviceContext_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), depthStencilView_.Get());


	// Set the viewport transform.

	screenViewport_.TopLeftX = 0;
	screenViewport_.TopLeftY = 0;
	screenViewport_.Width = static_cast<float>(screenWidth_);
	screenViewport_.Height = static_cast<float>(screenHeight_);
	screenViewport_.MinDepth = 0.0f;
	screenViewport_.MaxDepth = 1.0f;

	deviceContext_->RSSetViewports(1, &screenViewport_);
}

LRESULT D3DApp::handleWindowsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			appPaused_ = true;
			timer_.stop();
		}
		else
		{
			appPaused_ = false;
			timer_.start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		screenWidth_ = LOWORD(lParam);
		screenHeight_ = HIWORD(lParam);
		if (device_)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				appPaused_ = true;
				minimized_ = true;
				maximized_ = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				appPaused_ = false;
				minimized_ = false;
				maximized_ = true;
				onResized();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (minimized_)
				{
					appPaused_ = false;
					minimized_ = false;
					onResized();
				}

				// Restoring from maximized state?
				else if (maximized_)
				{
					appPaused_ = false;
					maximized_ = false;
					onResized();
				}
				else if (resizing_)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					onResized();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		appPaused_ = true;
		resizing_ = true;
		timer_.stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		appPaused_ = false;
		resizing_ = false;
		timer_.start();
		onResized();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::initMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = appInstance_;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"D3DWndClassName";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, screenWidth_, screenHeight_ };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mainWindow_ = CreateWindow(L"D3DWndClassName", mainWindowCaption_.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, appInstance_, 0);
	if (!mainWindow_)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mainWindow_, SW_SHOW);
	UpdateWindow(mainWindow_);

	return true;
}

bool D3DApp::initDirect3D()
{
	// Create the device and device context.
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
		nullptr,              // default adapter
		driverType_,
		nullptr,              // no software device
		createDeviceFlags,
		nullptr,              // default feature level array
		0,
		D3D11_SDK_VERSION,
		device_.GetAddressOf(),
		&featureLevel,
		deviceContext_.GetAddressOf());

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}

	if (featureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.
	HR(device_->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaaQuality_));
	assert(msaaQuality_ > 0);

	// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = screenWidth_;
	sd.BufferDesc.Height = screenHeight_;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Use 4X MSAA? 
	if (enable4xMsaa_)
	{
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = msaaQuality_ - 1;
	}
	else
	{
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = mainWindow_;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	// To correctly create the swap chain, we must use the IDXGIFactory that was
	// used to create the device.  If we tried to use a different IDXGIFactory instance
	// (by calling CreateDXGIFactory), we get an error: "IDXGIFactory::CreateSwapChain: 
	// This function is being called with a device from a different IDXGIFactory."
	ComPtr<IDXGIDevice> dxgiDevice;
	HR(device_->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf())));

	ComPtr<IDXGIAdapter> dxgiAdapter;
	HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(dxgiAdapter.GetAddressOf())));

	ComPtr<IDXGIFactory> dxgiFactory;
	HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(dxgiFactory.GetAddressOf())));

	HR(dxgiFactory->CreateSwapChain(device_.Get(), &sd, swapChain_.GetAddressOf()));

	// The remaining steps that need to be carried out for D3D creation
	// also need to be executed every time the window is resized.  So
	// just call the OnResize method here to avoid code duplication.
	onResized();

	return true;
}

#include <sstream> // Add this include directive at the top of the file

void D3DApp::calculateFrameStats()
{
    // Code computes the average frames per second, and also the 
    // average time it takes to render one frame.  These stats 
    // are appended to the window caption bar.

    static int frameCnt = 0;
    static float timeElapsed = 0.0f;

    frameCnt++;

    // Compute averages over one second period.
    if ((timer_.getTotalTime() - timeElapsed) >= 1.0f)
    {
        float fps = (float)frameCnt; // fps = frameCnt / 1
        float mspf = 1000.0f / fps;

        std::wostringstream outs{};
        outs.precision(6);
        outs << mainWindowCaption_ << L"    "
             << L"FPS: " << fps << L"    "
             << L"Frame Time: " << mspf << L" (ms)";
        SetWindowText(mainWindow_, outs.str().c_str());

        // Reset for next average.
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}
