#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "D3DApp.h"

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	bool Init();
	void OnResize();
	void updateScene(float dt) override;
	void drawScene() override;
};

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	InitDirect3DApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.run();
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::Init()
{
	if (!D3DApp::init())
		return false;

	return true;
}

void InitDirect3DApp::OnResize()
{
	D3DApp::onResized();
}

void InitDirect3DApp::updateScene(float dt)
{

}

void InitDirect3DApp::drawScene()
{
	assert(device_);
	assert(swapChain_);

	float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f }; // Example RGBA values for blue
	deviceContext_->ClearRenderTargetView(renderTargetView_.Get(), color);
	deviceContext_->ClearDepthStencilView(depthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	HR(swapChain_->Present(0, 0));
}
