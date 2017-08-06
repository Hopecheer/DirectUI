#include "pch.h"
#include "Window.h"
#include "Application.h"
#include "Colors.h"
#include "Buttons.h"

using namespace DirectUI;
using namespace DX;

Window* Window::_current = nullptr;

DEFINE_DP_WITH_FLAGS(Window, ClearColor, Color, Colors::BlueViolet(), PropertyMetadataFlags::AffectsRender);

Window::Window(const wchar_t* title) {
	_hWnd = ::CreateWindowW(DirectUIWindowClassName, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr, nullptr, ::GetModuleHandle(nullptr), nullptr);

	if (!_hWnd)
		throw std::exception("Failed to create window");

	SetWindow(this);
	Application::Current()->AddWindow(_hWnd, this);

	ContentProperty.RegisterPropertyChangedHandler([this](auto&, auto& oldContent, auto& newContent) {
		if (oldContent) {
			oldContent->SetParent(nullptr);
			oldContent->SetWindow(nullptr);
		}
		if (newContent) {
			newContent->SetParent(this);
			newContent->SetWindow(this);
		}
	});
}

bool Window::ResizeSwapChainBitmap()
{
	_dc.SetTarget();

	if (S_OK == _swapChain.ResizeBuffers()) {
		Application::CreateDeviceSwapChainBitmap(_swapChain, _dc);
		return true;
	}
	else
	{
		ReleaseDevice();
		return false;
	}
}

void Window::ReleaseDevice() {
	_dc.Reset();
	_swapChain.Reset();
}

LRESULT Window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) {
	using namespace std;

	switch (message) {
	case WM_PAINT:
		PAINTSTRUCT ps;
		::BeginPaint(_hWnd, &ps);
		Render();
		::EndPaint(_hWnd, &ps);
		break;

	case WM_DISPLAYCHANGE:
		Render();
		break;

	case WM_DESTROY:
		Application::Current()->RemoveWindow(_hWnd);

		break;

	case WM_LBUTTONDOWN: {
		auto args = GetMouseEventArgs(wParam, lParam);
		auto source = get<0>(args);
		source->OnMouseDown(*source, get<1>(args));
		break;
	}

	case WM_SIZE:
		if (SIZE_MINIMIZED != wParam)
		{
			if (ResizeSwapChainBitmap()) {
				Render();
			}
		}
		break;
	}

	return ::DefWindowProc(_hWnd, message, wParam, lParam);
}

std::tuple<UIElement*, MouseEventArgs> Window::GetMouseEventArgs(WPARAM wParam, LPARAM lParam) {
	MouseEventArgs args;
	args.Button = static_cast<MouseButton>(wParam & static_cast<WPARAM>(MouseButton::AllButtons));
	int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
	Point2F point(static_cast<float>(x), static_cast<float>(y));

	UIElement* source = this;
	auto content = Content();
	if (content) {
		source = content->HitTest(point);
	}
	if (source == nullptr)
		source = this;

	return std::make_tuple(source, args);
}

void Window::Render() {
	using namespace DX;

	if (!_dc) {
		bool success = Application::Current()->CreateDeviceContextAndSwapChain(_hWnd, _dc, _swapChain);
		ASSERT(success);
	}

	_current = this;

	auto content = Content();
	if (content)
		content->Measure(_dc.GetSize());

	_dc.BeginDraw();
	_dc.Clear(ClearColor());
	Draw(_dc);
	_dc.EndDraw();

	_current = nullptr;

	_swapChain.Present();
}

void Window::Invalidate(const DX::RectF & rect) {
	RECT rc = { (int)rect.Left, (int)rect.Top, (int)rect.Right, (int)rect.Bottom };
	::InvalidateRect(_hWnd, &rc, FALSE);
}

void Window::Draw(Direct2D::DeviceContext& dc) {
	auto size = dc.GetSize();
	auto& content = Content();
	if (content)
		content->Draw(dc, RectF(0, 0, size.Width, size.Height));
}
