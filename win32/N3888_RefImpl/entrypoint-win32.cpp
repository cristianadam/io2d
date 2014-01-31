// entrypoint-win32.cpp : Defines the entry point for the application.
//

#include "entrypoint-win32.h"

#include "cairo.h"
#include "cairo-win32.h"

#include <memory>
#include <functional>
#include <atomic>
#include <wrl.h>
#include "throw_helpers.h"
#include "drawing.h"
#include "Win32RenderWindow.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace Microsoft::WRL;
using namespace std;
using namespace std::experimental;
using namespace std::experimental::drawing;

#define MAX_LOADSTRING 100

class ref_counted_bool {
	::std::atomic<int> m_refCount;
public:
	ref_counted_bool() : m_refCount(0) {}
	ref_counted_bool& operator=(bool value) {
		if (value) {
			m_refCount++;
		}
		else {
			auto count = m_refCount.load(memory_order_acquire);
			if (--count < 0) {
				count = 0;
			}
			m_refCount.store(count, memory_order_release);
		}
		return *this;
	}
	operator bool() {
		return m_refCount.load() != 0;
	}
};

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
unique_ptr<surface> g_psurface;
RECT g_previousClientRect;
ref_counted_bool g_doNotPaint;

// Everything in the Draw function should be portable C++ code.
void Draw(surface& surface) {
	// Create a context that will draw to our surface.
	auto context = drawing::context(surface);

	// The is a demonstration of how a raster_source_pattern works. We create one that is 100px x 100px
	//auto pattern = raster_source_pattern(nullptr, content::color_alpha, 100, 100);
	//pattern.set_acquire(
	//	[pattern](void*, experimental::drawing::surface& target, const rectangle_int& extents) -> experimental::drawing::surface
	//{
	//	auto result = experimental::drawing::image_surface(target, format::rgb24, extents.width - extents.x, extents.height - extents.y);
	//	vector<unsigned char> data;
	//	const auto dataSize = result.get_stride() * result.get_height();
	//	data.resize(dataSize);
	//	for (auto i = 0; i < dataSize; i += 4) {
	//		data[i + 0] = 255ui8;
	//		data[i + 1] = 0ui8;
	//		data[i + 2] = 0ui8;
	//		data[i + 3] = 0ui8;
	//	}
	//	result.set_data(data);
	//	return result;
	//},
	//	nullptr
	//	);
	//pattern.set_extend(extend::repeat);
	//context.set_source(pattern);
	//context.paint();

	context.save();
	auto scp = solid_color_pattern(0.0, 0.0, 1.0);
	context.set_source(scp);
	context.paint();
	context.restore();

	context.save();
	const int width = 100;
	const int height = 100;
	const format fmt = format::rgb24;
	const int stride = format_stride_for_width(fmt, width);
	vector<unsigned char> data;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < stride; x++) {
			auto byte = x % 4;
			switch (byte)
			{
			case 0:
				data.push_back(0x7Fui8);
				break;
			case 1:
				data.push_back(0xFFui8);
				break;
			case 2:
				data.push_back(0ui8);
				break;
			case 3:
				data.push_back(0ui8);
				break;
			default:
				throw logic_error("We're MODing by 4, how do we have a value outside of [0,3]?");
			}
		}
	}
	auto imageSurfaceFromData = image_surface(data, fmt, width, height, stride);
	context.set_source_surface(imageSurfaceFromData, 400.0, 400.0);
	context.move_to(400.0, 400.0);
	context.rel_line_to(100.0, 0.0);
	context.rel_line_to(0.0, 100.0);
	context.rel_line_to(-100.0, 0.0);
	context.close_path();
	context.fill();
	imageSurfaceFromData.finish();
	context.restore();

	context.save();
	auto surfaceForContext2 = image_surface(format::argb32, 100, 100);
	auto context2 = drawing::context(surfaceForContext2);
	context2.set_source_rgba(0.5, 0.5, 0.5, 0.5);
	context2.set_compositing_operator(compositing_operator::clear);
	context2.paint_with_alpha(0.0);
	context2.set_source_rgba(0.5, 0.5, 0.5, 0.5);
	context2.set_compositing_operator(compositing_operator::add);
	context2.paint();
	surfaceForContext2.flush();
	context.set_source_surface(surfaceForContext2, 600.0, 400.0);
	context.move_to(600.0, 400.0);
	context.rel_line_to(100.0, 0.0);
	context.rel_line_to(0.0, 100.0);
	context.rel_line_to(-100.0, 0.0);
	context.close_path();
	context.fill();
	context.set_source_surface(surfaceForContext2, 650.0, 400.0);
	context.move_to(650.0, 400.0);
	context.rel_line_to(100.0, 0.0);
	context.rel_line_to(0.0, 100.0);
	context.rel_line_to(-100.0, 0.0);
	context.close_path();
	context.fill();
	context.restore();

	context.save();
	auto subsurface = drawing::surface(surface, 10.5, 11.0, 50.0, 50.0);
	auto subcontext = drawing::context(subsurface);
	subcontext.set_source_rgb(0.0, 0.0, 0.0);
	subcontext.move_to(2.0, 2.0);
	subcontext.rel_line_to(48.0, 0.0);
	subcontext.set_line_width(3.0);
	subcontext.set_line_cap(line_cap::butt);
	subcontext.stroke();
	context.restore();

	context.save();
	matrix m;
	m.init_translate(300.0, 400.0);
	const double two_pi = 3.1415926535897932 * 2.0;
	m.rotate(two_pi * ((GetTickCount64() % 4000) / 4000.0));
	context.set_matrix(m);
	context.new_path();
	context.move_to(-100.0, 0.0);
	context.line_to(100.0, 0.0);
	context.line_to(0.0, 200.0);
	context.close_path();
	context.set_line_width(3.0);
	context.set_dash({ 0.0, 10.0 }, 0.0);
	context.set_line_cap(line_cap::round);

	// Create a copy of the path because when we call fill it clears from the context.
	auto path = context.copy_path();
	context.set_source_rgb(1.0, 0.0, 0.0);
	context.fill();

	// Use the same path so that we can outline it.
	context.append_path(path);
	context.set_source_rgb(0.0, 0.0, 0.0);
	context.stroke();
	context.restore();

	context.set_source_rgb(1.0, 1.0, 1.0);
	context.move_to(100.0, 100.0);
	context.select_font_face("Segoe UI", font_slant::normal, font_weight::normal);
	context.set_font_size(30.0);
	context.show_text("Hello C++!");
}

void OnPaint(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	PAINTSTRUCT ps;
	HDC hdc;
	RECT updateRect{ };
	auto getUpdateRectResult = GetUpdateRect(hWnd, &updateRect, FALSE);

	// There's a bug somewhere (possibly cairo?) where if you run this code without the check to make sure that
	// updateRect.left and .top are both 0, it crashes and does so in the cairo DLL such that it's immune to
	// being caught. Specifically in a Win32/Debug config it throws well inside cairo on the ctxt.paint() call
	// on an illegal memory access (actually in pixman-sse2.c in a call to "void save_128_aligned(__m128i*, __m128i);").
	if (getUpdateRectResult != FALSE && updateRect.left == 0 && updateRect.top == 0) {
		hdc = BeginPaint(hWnd, &ps);

		RECT clientRect;
		if (!GetClientRect(hWnd, &clientRect)) {
			throw_get_last_error<logic_error>("Failed GetClientRect call.");
		}
		auto width = clientRect.right - clientRect.left;
		auto height = clientRect.bottom - clientRect.top;
		auto previousWidth = g_previousClientRect.right - g_previousClientRect.left;
		auto previousHeight = g_previousClientRect.bottom - g_previousClientRect.top;

		// To enable screenshot saving, we are using a global unique_ptr surface. I did not rewrite the boilerplate
		// Win32 code so that it'd be a class, hence the globals.
		if ((g_psurface == nullptr) || (width != previousWidth) || (height != previousHeight)) {
			g_psurface = unique_ptr<surface>(new surface(move(make_surface(format::argb32, width, height))));
			g_previousClientRect = clientRect;
		}

		// Draw to the off-screen buffer.
		Draw(*g_psurface);

		// Flush to ensure that it is drawn to the window.
		g_psurface->flush();

		auto surface = make_surface(cairo_win32_surface_create(hdc));
		auto ctxt = context(surface);
		ctxt.set_source_surface(*g_psurface, 0.0, 0.0);
		ctxt.paint();
		surface.flush();
		EndPaint(hWnd, &ps);
	}
	else {
		if (getUpdateRectResult != FALSE) {
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
	}
}

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR lpCmdLine,
	_In_ int nCmdShow
	) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	g_previousClientRect = { }; // Zero out previous client rect.
	throw_if_failed_hresult<runtime_error>(
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED), "Failed call to CoInitializeEx."
		);
	MSG msg{ };
	msg.message = WM_NULL;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_N3888_REFIMPL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	HWND hWnd;
	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow, hWnd)) {
		CoUninitialize();
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_N3888_REFIMPL));

	Win32RenderWindow window( 320, 240, L"N3888_RefImpl rocks!" );
	Win32RenderWindow window2(320, 240, L"N3888_RefImpl rocks two times!");

	// Main message loop:
	while (msg.message != WM_QUIT) {
		if (!PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
			// No message so redraw the window.
			if (g_psurface != nullptr && !g_doNotPaint) {
				RECT clientRect;
				if (!GetClientRect(hWnd, &clientRect)) {
					throw_get_last_error<logic_error>("Failed GetClientRect call.");
				}
				auto width = clientRect.right - clientRect.left;
				auto height = clientRect.bottom - clientRect.top;
				auto previousWidth = g_previousClientRect.right - g_previousClientRect.left;
				auto previousHeight = g_previousClientRect.bottom - g_previousClientRect.top;

				// To enable screenshot saving, we are using a global unique_ptr surface. I did not rewrite the boilerplate
				// Win32 code so that it'd be a class, hence the globals.
				if ((width != previousWidth) || (height != previousHeight)) {
					g_psurface = unique_ptr<surface>(new surface(move(make_surface(format::argb32, width, height))));
					g_previousClientRect = clientRect;
				}

				// Draw to the off-screen buffer.
				Draw(*g_psurface);

				// Flush to ensure that it is drawn to the window.
				g_psurface->flush();
				auto hdc = GetDC(hWnd);
				{
					auto surface = make_surface(cairo_win32_surface_create(hdc));
					auto ctxt = context(surface);
					ctxt.set_source_surface(*g_psurface, 0.0, 0.0);
					ctxt.paint();
					surface.flush();
				}
				ReleaseDC(hWnd, hdc);
			}
		}
		else {
			if (msg.message != WM_QUIT) {
				if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
	}
	CoUninitialize();
	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEX wcex{ };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_N3888_REFIMPL));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_N3888_REFIMPL);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hWnd) {
	//HWND hWnd;
	INITCOMMONCONTROLSEX initCommonControlsEx{ };
	initCommonControlsEx.dwSize = sizeof(initCommonControlsEx);
	initCommonControlsEx.dwICC = ICC_LINK_CLASS;
	if (InitCommonControlsEx(&initCommonControlsEx) == FALSE) {
		throw runtime_error("Failed call to InitCommonControlsEx.");
	}

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

void ShowSaveAsPNGDialog() {
	ComPtr<IFileSaveDialog> fsd;
	throw_if_failed_hresult<logic_error>(
		CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fsd)), "Failed call to CoCreateInstance for IFileSaveDialog.");

	FILEOPENDIALOGOPTIONS fodOptions{ };
	throw_if_failed_hresult<logic_error>(
		fsd->GetOptions(&fodOptions), "Failed call to IFileDialog::GetOptions.");
	throw_if_failed_hresult<logic_error>(
		fsd->SetOptions(fodOptions | FOS_FORCEFILESYSTEM | FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST), "Failed call to IFileDialog::SetOptions.");

	const COMDLG_FILTERSPEC filterSpec[] = {
		{ L"PNG", L".png" }
	};
	throw_if_failed_hresult<logic_error>(
		fsd->SetFileTypes(ARRAYSIZE(filterSpec), filterSpec), "Failed call to IFileDialog::SetFileTypes.");
	throw_if_failed_hresult<logic_error>(
		fsd->SetFileTypeIndex(1U), "Failed call to IFileDialog::SetFileTypeIndex.");
	throw_if_failed_hresult<logic_error>(
		fsd->SetDefaultExtension(L"png"), "Failed call to IFileDialog::SetDefaultExtension.");

	ComPtr<IKnownFolderManager> kfm;
	throw_if_failed_hresult<logic_error>(
		CoCreateInstance(CLSID_KnownFolderManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&kfm)), "Failed call to CoCreateInstance for IKnownFolderManager.");
	ComPtr<IKnownFolder> picturesKnownFolder;
	throw_if_failed_hresult<logic_error>(
		kfm->GetFolder(FOLDERID_Pictures, &picturesKnownFolder), "Failed call to IKnownFolderManager::GetFolder.");
	ComPtr<IShellItem> picturesShellItem;
	throw_if_failed_hresult<logic_error>(
		picturesKnownFolder->GetShellItem(0, IID_PPV_ARGS(&picturesShellItem)), "Failed call to IKnownFolder::GetShellItem.");

	throw_if_failed_hresult<logic_error>(
		fsd->SetDefaultFolder(picturesShellItem.Get()), "Failed call to IFileDialog::SetDefaultFolder.");

	HRESULT hr;
	hr = fsd->Show(nullptr);
	if (SUCCEEDED(hr)) {
		// The user picked a file.
		ComPtr<IShellItem> result;
		throw_if_failed_hresult<logic_error>(
			fsd->GetResult(&result), "Failed call to IFileDialog::GetResult.");
		wstring wfilename;
		PWSTR pwstrFilename = nullptr;
		throw_if_failed_hresult<logic_error>(
			result->GetDisplayName(SIGDN_FILESYSPATH, &pwstrFilename), "Failed call to IShellItem::GetDisplayName.");
		try {
			wfilename = pwstrFilename;
			CoTaskMemFree(pwstrFilename);
		}
		catch (...) {
			CoTaskMemFree(pwstrFilename);
			throw;
		}
		HANDLE hFile = CreateFileW(wfilename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			throw_get_last_error<runtime_error>("Failed call to CreateFileW.");
		}
		if (CloseHandle(hFile) == 0) {
			throw_get_last_error<runtime_error>("Failed call to CloseHandle.");
		}
		auto bufferSize = GetShortPathNameW(wfilename.c_str(), nullptr, 0);
		wstring wshortfilename;
		wshortfilename.resize(bufferSize);
		if (GetShortPathNameW(wfilename.c_str(), &wshortfilename[0], bufferSize) == 0) {
			throw_get_last_error<runtime_error>("Failed call to GetShortPathNameW.");
		}
		char defaultChar = '*';
		BOOL usedDefault = FALSE;
		auto mbBufferSize = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wshortfilename.c_str(), -1, nullptr, 0, &defaultChar, &usedDefault);
		string mbFileName;
		usedDefault = FALSE;
		mbFileName.resize(mbBufferSize);
		if (WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wshortfilename.c_str(), -1, &mbFileName[0], mbBufferSize, &defaultChar, &usedDefault) == 0) {
			throw_get_last_error<runtime_error>("Failed call to WideCharToMultiByte.");
		}
		if (usedDefault != FALSE) {
			throw runtime_error("Could not convert short filename string to multibyte from wide character.");
		}
		g_psurface->write_to_png(mbFileName);
	}
	else {
		if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
			// Do nothing. The user clicked cancel.
		}
		else {
			throw_if_failed_hresult<logic_error>(hr, "Failed call to IModalWindow::Show.");
		}
	}

}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int wmId, wmEvent;

#if defined(DEBUG_WNDPROC)
	wstringstream str;
	str << L"Message: 0x" << hex << uppercase << message << nouppercase
		<< L". WPARAM: 0x" << hex << uppercase << static_cast<UINT>(wParam) << nouppercase
		<< L". LPARAM: 0x" << hex << uppercase << static_cast<UINT>(lParam) << endl;
	OutputDebugStringW(str.str().c_str());
#endif

	switch (message) {
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId) {
		case IDM_ABOUT:
		{
			auto aboutResult = DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			if (aboutResult <= 0) {
				throw_get_last_error<logic_error>("Failed call to DialogBox.");
			}
		}
			break;
		case ID_EDIT_SCREENCAPTURE:
			ShowSaveAsPNGDialog();
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_ENTERSIZEMOVE:
		g_doNotPaint = true; // Don't paint while resizing to avoid flicker.
		return DefWindowProc(hWnd, message, wParam, lParam);
	case WM_EXITSIZEMOVE:
		g_doNotPaint = false;
		return DefWindowProc(hWnd, message, wParam, lParam);
	case WM_PAINT:
		if (!g_doNotPaint) {
			OnPaint(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	case WM_NOTIFY:
	{
		PNMLINK pnmLink = reinterpret_cast<PNMLINK>(lParam);
		if ((pnmLink->hdr.idFrom == IDC_SYSLINK1) || (pnmLink->hdr.idFrom == IDC_SYSLINK2)) {
			switch (pnmLink->hdr.code)
			{
			case NM_CLICK:
				// Intentional fall-through.
			case NM_RETURN:
			{
				auto shExecResult = reinterpret_cast<int>(ShellExecute(nullptr, L"open", pnmLink->item.szUrl, nullptr, nullptr, SW_SHOW));
				if (shExecResult <= 32) {
					wstringstream err;
					err << L"Error calling ShellExecute while trying to open the link. Return code: " << to_wstring(shExecResult) << "." << endl;
					MessageBox(hDlg, err.str().c_str(), L"Error opening link", MB_OK | MB_ICONEXCLAMATION);
				}
			}
				return (INT_PTR)TRUE;
			}
		}
	}
		break;
	}
	return (INT_PTR)FALSE;
}