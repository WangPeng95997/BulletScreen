#include <Windows.h>
#define ID_EDIT 1001
#define ID_SEND 1002
#define COLOR 3
#define COUNT 7

// 红, 橙, 黄, 绿, 青, 蓝, 紫
COLORREF dwColor[COUNT][COLOR] = { {255, 0, 0}, {255, 128, 0}, {255, 255, 0}, {0, 255, 0}, {0, 255, 255}, {0, 0, 255}, {128, 0, 128} };
HFONT g_hFont;
HINSTANCE g_hInst;
HWND g_hwndMain, g_hwndEdit, g_hwndSend;
HDC g_hdcEdit, g_hdcSend;

// 随机数函数
HCRYPTPROV hProv;
int Random()
{
	if (hProv == 0)
		if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_SILENT | CRYPT_VERIFYCONTEXT))
			return 0;

	int out = 0;
	CryptGenRandom(hProv, sizeof(out), (BYTE*)(&out));

	return out & 0x7FFFFFFF;
}

// 设置窗口的显示内容和实现弹幕从右到左的移动效果
DWORD WINAPI MoveBulletScreen(LPVOID lpParameter)
{
	RECT rekt;
	SIZE size;
	POINT ptSrc;
	ZeroMemory(&rekt, sizeof(RECT));
	ZeroMemory(&size, sizeof(SIZE));
	ZeroMemory(&ptSrc, sizeof(POINT));

	HWND hwnd = *(HWND*)lpParameter;
	HDC hdc = GetDC(hwnd);
	HDC hdcPaint = CreateCompatibleDC(hdc);
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);
	int color = Random() % COUNT;

	SetTextColor(hdcPaint, RGB(dwColor[color][0], dwColor[color][1], dwColor[color][2]));
	SetBkColor(hdcPaint, RGB(0, 0, 0));
	SelectObject(hdcPaint, g_hFont);
	GetTextExtentPoint32(hdcPaint, (WCHAR*)lpParameter + 2, wcslen((WCHAR*)lpParameter + 2), &size);
	MoveWindow(hwnd, width, (height = Random() % height - 100) > 0 ? height : 0, size.cx, size.cy, FALSE);

	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, size.cx, size.cy);
	HGDIOBJ hGdiobj = SelectObject(hdcPaint, hBitmap);
	TextOut(hdcPaint, 0, 0, (WCHAR*)lpParameter + 2, wcslen((WCHAR*)lpParameter + 2));
	UpdateLayeredWindow(hwnd, NULL, NULL, &size, hdcPaint, &ptSrc, 0, NULL, ULW_COLORKEY);
	SetForegroundWindow(g_hwndMain);

	do
	{
		GetWindowRect(hwnd, &rekt);
		MoveWindow(hwnd, rekt.left - 3, rekt.top, rekt.right - rekt.left, rekt.bottom - rekt.top, FALSE);
		Sleep(20);
	} while (rekt.right > 0);
	SelectObject(hdcPaint, hGdiobj);
	DeleteObject(hBitmap);
	DeleteDC(hdcPaint);
	ReleaseDC(hwnd, hdc);

	return PostMessage(hwnd, WM_QUIT, 0, 0);
}

// 创建弹幕窗口及对应的消息循环
DWORD WINAPI BulletScreenWindow(LPVOID lpParameter)
{
	HWND hwnd = *(HWND*)lpParameter = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, L"BulletScreen", L"", WS_POPUP, 0, 0, 100, 100, NULL, NULL, g_hInst, NULL);
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	HANDLE hThread = CreateThread(NULL, 0, MoveBulletScreen, lpParameter, 0, NULL);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	LocalFree(lpParameter);

	return hThread ? CloseHandle(hThread) : 0;
}

// 点击 "发送弹幕" 按钮时的处理函数
int OnSend(HWND hwnd)
{
	LRESULT lRet = SendMessage(g_hwndEdit, WM_GETTEXTLENGTH, 0, 0);

	if (lRet++)
	{
		LPVOID* lpSzText = (LPVOID*)LocalAlloc(LMEM_ZEROINIT, sizeof(LPVOID*) * (lRet + 1));
		SendMessage(g_hwndEdit, WM_GETTEXT, lRet, (LPARAM)((WCHAR*)lpSzText + 2));
		SendMessage(g_hwndEdit, WM_SETTEXT, 0, (LPARAM)L"");

		HANDLE hThread = CreateThread(NULL, 0, BulletScreenWindow, lpSzText, 0, NULL);
		return hThread ? CloseHandle(hThread) : 0;
	}
	else
		return MessageBox(hwnd, L"请输入弹幕", L"错误", MB_OK);
}

// 主窗口的WM_COMMAND消息处理函数
void OnCommand(HWND hwnd, WPARAM wParam)
{
	switch (LOWORD(wParam))
	{
	case ID_EDIT:
		break;

	case ID_SEND:
		OnSend(hwnd);
		break;
	}
}

// 主窗口的WM_CREATE消息处理函数
void OnCreate(HWND hwnd)
{
	g_hFont = CreateFont(40, 0, 0, 1800, 0, 0, 0, 0, GB2312_CHARSET, 0, 0, 0, 0, L"微软雅黑");
	g_hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 68, 50, 250, 50, hwnd, (HMENU)ID_EDIT, g_hInst, NULL);
	g_hwndSend = CreateWindowEx(0, L"BUTTON", L"发送弹幕", WS_CHILD | WS_VISIBLE, 118, 150, 150, 50, hwnd, (HMENU)ID_SEND, g_hInst, NULL);
	g_hdcEdit = GetDC(g_hwndEdit);
	g_hdcSend = GetDC(g_hwndSend);
	SendMessage(g_hwndEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
	SendMessage(g_hwndSend, WM_SETFONT, (WPARAM)g_hFont, TRUE);
}

// 主窗口的回调函数
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		OnCreate(hwnd);
		break;

	case WM_COMMAND:
		OnCommand(hwnd, wParam);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 弹幕窗口的回调函数
LRESULT CALLBACK BulletScreenProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

ATOM RegisterWindowClass(HINSTANCE hInstance, WNDPROC lpWndProc, LPCWSTR g_lpClassName)
{
	WNDCLASSEX WndEx;
	WndEx.cbSize = sizeof(WNDCLASSEX);
	WndEx.style = CS_HREDRAW | CS_VREDRAW;
	WndEx.lpfnWndProc = lpWndProc;
	WndEx.cbClsExtra = 0;
	WndEx.cbWndExtra = 0;
	WndEx.hInstance = hInstance;
	WndEx.hIcon = NULL;
	WndEx.hCursor = NULL;
	WndEx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndEx.lpszMenuName = NULL;
	WndEx.lpszClassName = g_lpClassName;
	WndEx.hIconSm = NULL;

	return RegisterClassEx(&WndEx);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	g_hInst = hInstance;

	// 注册窗口类
	if (!RegisterWindowClass(hInstance, (WNDPROC)MainWindowProc, L"MainWindow") || !RegisterWindowClass(hInstance, (WNDPROC)BulletScreenProc, L"BulletScreen"))
		return MessageBox(NULL, L"窗口类注册失败", L"错误", MB_OK);

	// 创建主窗口
	g_hwndMain = CreateWindowEx(0, L"MainWindow", L"弹幕程序", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, NULL, hInstance, NULL);
	ShowWindow(g_hwndMain, SW_SHOW);
	UpdateWindow(g_hwndMain);

	// 主窗口消息循环
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// 注销窗口类和删除创建的字体
	UnregisterClass(L"MainWindow", hInstance);
	UnregisterClass(L"BulletScreen", hInstance);
	DeleteObject(g_hFont);

	return 0;
}