#pragma once
#include "gina_shutdownview.h"
#include <vector>
#include "../util/util.h"
#include "../util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <powrprof.h>

#pragma comment(lib, "PowrProf.lib")

std::atomic<bool> isShutdownViewActive(false);
std::atomic<bool> isLogoffViewActive(false);

std::mutex shutdownViewMutex;
std::mutex logoffViewMutex;

void ShowShutdownDialog(HWND parent)
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}
	std::lock_guard<std::mutex> lock(shutdownViewMutex);
	
	std::thread([=] {
		if (isShutdownViewActive.exchange(true)) {
			return;
		}
		if (parent) {
			EnableWindow(parent, FALSE);
		}
		ginaShutdownView::Get()->Create(parent);
		ginaShutdownView::Get()->Show();
		ginaShutdownView::Get()->BeginMessageLoop();
		isShutdownViewActive = false;
		if (parent) {
			EnableWindow(parent, TRUE);
		}
	}).detach();
}

void ShowLogoffDialog(HWND parent)
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}
	std::lock_guard<std::mutex> lock(logoffViewMutex);
	
	std::thread([=] {
		if (isLogoffViewActive.exchange(true)) {
			return;
		}
		if (parent) {
			EnableWindow(parent, FALSE);
		}
		ginaLogoffView::Get()->Create(parent);
		ginaLogoffView::Get()->Show();
		ginaLogoffView::Get()->BeginMessageLoop();
		isLogoffViewActive = false;
		if (parent) {
			EnableWindow(parent, TRUE);
		}
	}).detach();
}

ginaShutdownView* ginaShutdownView::Get()
{
	static ginaShutdownView dlg;
	return &dlg;
}

void ginaShutdownView::Create(HWND parent)
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaShutdownView::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_SHUTDOWN), parent, (DLGPROC)DlgProc, 0);
	if (!ginaShutdownView::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create shutdown dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		return;
	}
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaShutdownView::Get()->hDlg);
	}
}

void ginaShutdownView::Destroy()
{
	ginaShutdownView* dlg = ginaShutdownView::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaShutdownView::Show()
{
	ginaShutdownView* dlg = ginaShutdownView::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
	UpdateWindow(dlg->hDlg);
}

void ginaShutdownView::Hide()
{
	ginaShutdownView* dlg = ginaShutdownView::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaShutdownView::BeginMessageLoop()
{
	ginaShutdownView* dlg = ginaShutdownView::Get();
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_KEYDOWN)
		{
			switch (msg.wParam)
			{
			case VK_RETURN:
			{
				if (!TabSpace(dlg->hDlg, shutdownTabIndex, sizeof(shutdownTabIndex) / sizeof(shutdownTabIndex[0])))
				{
					SendMessage(dlg->hDlg, WM_COMMAND, MAKEWPARAM(IDC_OK, BN_CLICKED), 0);
				}
			}
			case VK_SPACE:
			{
				TabSpace(dlg->hDlg, shutdownTabIndex, sizeof(shutdownTabIndex) / sizeof(shutdownTabIndex[0]));
				break;
			}
			case VK_ESCAPE:
			{
				SendMessage(dlg->hDlg, WM_COMMAND, MAKEWPARAM(IDC_CANCEL, BN_CLICKED), 0);
				break;
			}
			case VK_TAB:
			{
				if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
				{
					TabPrev(dlg->hDlg, shutdownTabIndex, sizeof(shutdownTabIndex) / sizeof(shutdownTabIndex[0]), IDC_OK);
				}
				else
				{
					TabNext(dlg->hDlg, shutdownTabIndex, sizeof(shutdownTabIndex) / sizeof(shutdownTabIndex[0]), IDC_OK);
				}
				break;
			}
			}
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

int CALLBACK ginaShutdownView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;

		HWND hShutdownIcon = GetDlgItem(hWnd, IDC_SHUTDOWN_ICON);
		SendMessageW(hShutdownIcon, STM_SETICON, (WPARAM)LoadIconW(hGinaDll, MAKEINTRESOURCEW(IDI_SHUTDOWN)), 0);

		// Populate shutdown combo
		HWND hShutdownCombo = GetDlgItem(hWnd, IDC_SHUTDOWN_COMBO);
		wchar_t shutdownStr[256];
		if (!IsSystemUser())
		{
			wchar_t logoffStr[256], username[MAX_PATH], domain[MAX_PATH];
			GetLoggedOnUserInfo(username, MAX_PATH, domain, MAX_PATH);
			LoadStringW(hGinaDll, GINA_STR_LOGOFF, logoffStr, 256);
			swprintf_s(shutdownStr, logoffStr, username);
			SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		}
		LoadStringW(hGinaDll, GINA_STR_SHUTDOWN, shutdownStr, 256);
		SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		LoadStringW(hGinaDll, GINA_STR_RESTART, shutdownStr, 256);
		SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		LoadStringW(hGinaDll, GINA_STR_SLEEP, shutdownStr, 256);
		SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		LoadStringW(hGinaDll, GINA_STR_HIBERNATE, shutdownStr, 256);
		SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		SendMessageW(hShutdownCombo, CB_SETCURSEL, 2, 0);

		wchar_t shutdownDesc[256];
		LoadStringW(hGinaDll, GINA_STR_RESTART_DESC, shutdownDesc, 256);
		SetDlgItemTextW(hWnd, IDC_SHUTDOWN_DESC, shutdownDesc);

		// Hide help button and move the OK and Cancel buttons
		HWND hHelpBtn = GetDlgItem(hWnd, IDC_SHUTDOWN_HELP);
		ShowWindow(hHelpBtn, SW_HIDE);
		HWND hOkBtn = GetDlgItem(hWnd, IDC_OK);
		HWND hCancelBtn = GetDlgItem(hWnd, IDC_CANCEL);
		RECT helpRect, okRect, cancelRect;
		GetWindowRect(hHelpBtn, &helpRect);
		GetWindowRect(hOkBtn, &okRect);
		GetWindowRect(hCancelBtn, &cancelRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&helpRect, 2);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&okRect, 2);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&cancelRect, 2);
		SetWindowPos(hOkBtn, NULL, okRect.left + helpRect.right - helpRect.left + 8, okRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		SetWindowPos(hCancelBtn, NULL, cancelRect.left + helpRect.right - helpRect.left + 8, cancelRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		// Hide the shutdown tracker (XP only)
		HWND hShutdownTracker = GetDlgItem(hWnd, IDC_SHUTDOWN_TRACKER_GROUP);
		if (hShutdownTracker)
		{
			ShowWindow(hShutdownTracker, SW_HIDE);
			HWND hShutdownTrackerDesc = GetDlgItem(hWnd, IDC_SHUTDOWN_TRACKER_DESC);
			HWND hShutdownTrackerBooted = GetDlgItem(hWnd, IDC_SHUTDOWN_TRACKER_BOOTED);
			HWND hShutdownTrackerOptionsLabel = GetDlgItem(hWnd, IDC_SHUTDOWN_TRACKER_OPTIONS_LABEL);
			HWND hShutdownTrackerOptionsCombo = GetDlgItem(hWnd, IDC_SHUTDOWN_TRACKER_OPTIONS_COMBO);
			HWND hShutdownTrackerOptionsDesc = GetDlgItem(hWnd, IDC_SHUTDOWN_TRACKER_OPTIONS_DESC);
			HWND hShutdownTrackerDescLabel = GetDlgItem(hWnd, IDC_SHUTDOWN_TRACKER_DESC_LABEL);
			HWND hShutdownTrackerDescEdit = GetDlgItem(hWnd, IDC_SHUTDOWN_TRACKER_DESC_EDIT);
			ShowWindow(hShutdownTrackerDesc, SW_HIDE);
			ShowWindow(hShutdownTrackerBooted, SW_HIDE);
			ShowWindow(hShutdownTrackerOptionsLabel, SW_HIDE);
			ShowWindow(hShutdownTrackerOptionsCombo, SW_HIDE);
			ShowWindow(hShutdownTrackerOptionsDesc, SW_HIDE);
			ShowWindow(hShutdownTrackerDescLabel, SW_HIDE);
			ShowWindow(hShutdownTrackerDescEdit, SW_HIDE);

			RECT trackerRect;
			GetWindowRect(hShutdownTracker, &trackerRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&trackerRect, 2);
			int trackerHeight = trackerRect.bottom - trackerRect.top;
			RECT dlgRect;
			GetWindowRect(hWnd, &dlgRect);

			RECT okRect, cancelRect;
			HWND hOkBtn = GetDlgItem(hWnd, IDC_OK);
			HWND hCancelBtn = GetDlgItem(hWnd, IDC_CANCEL);
			GetWindowRect(hOkBtn, &okRect);
			GetWindowRect(hCancelBtn, &cancelRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&okRect, 2);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&cancelRect, 2);
			SetWindowPos(hOkBtn, NULL, okRect.left, okRect.top - trackerHeight, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			SetWindowPos(hCancelBtn, NULL, cancelRect.left, cancelRect.top - trackerHeight, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

			RECT dlgRectNew;
			GetWindowRect(hWnd, &dlgRectNew);
			int dlgHeight = dlgRectNew.bottom - dlgRectNew.top;
			SetWindowPos(hWnd, NULL, dlgRectNew.left, dlgRectNew.top, dlgRectNew.right - dlgRectNew.left, dlgHeight - trackerHeight, SWP_NOZORDER);
		}

		ginaManager::Get()->LoadBranding(hWnd, FALSE);
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			HWND hShutdownCombo = GetDlgItem(hWnd, IDC_SHUTDOWN_COMBO);
			int index = SendMessageW(hShutdownCombo, CB_GETCURSEL, 0, 0);
			if (IsSystemUser())
			{
				index += 1;
			}
			int descId = 0;
			switch (index)
			{
			case 0:
				descId = GINA_STR_LOGOFF_DESC;
				break;
			case 1:
				descId = GINA_STR_SHUTDOWN_DESC;
				break;
			case 2:
				descId = GINA_STR_RESTART_DESC;
				break;
			case 3:
				descId = GINA_STR_SLEEP_DESC;
				break;
			case 4:
				descId = GINA_STR_HIBERNATE_DESC;
				break;
			}
			wchar_t shutdownDesc[256];
			LoadStringW(ginaManager::Get()->hGinaDll, descId, shutdownDesc, 256);
			SetDlgItemTextW(hWnd, IDC_SHUTDOWN_DESC, shutdownDesc);
		}
		else if (LOWORD(wParam) == IDC_OK)
		{
			HWND hShutdownCombo = GetDlgItem(hWnd, IDC_SHUTDOWN_COMBO);
			int index = SendMessageW(hShutdownCombo, CB_GETCURSEL, 0, 0);
			if (IsSystemUser())
			{
				index += 1;
			}

			switch (index)
			{
			case 0:
				ExitWindowsEx(EWX_LOGOFF, 0);
				break;
			case 1:
				EnableShutdownPrivilege();
				ExitWindowsEx(EWX_SHUTDOWN, 0);
				break;
			case 2:
				EnableShutdownPrivilege();
				ExitWindowsEx(EWX_REBOOT, 0);
				break;
			case 3:
				SetSuspendState(FALSE, FALSE, FALSE);
				break;
			case 4:
				SetSuspendState(TRUE, FALSE, FALSE);
				break;
			}

			ginaShutdownView::Destroy();
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
		{
			ginaShutdownView::Destroy();
		}
		break;
	}
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		int origBottom = rect.bottom;
		rect.bottom = GINA_SMALL_BRD_HEIGHT;
		COLORREF brdColor = RGB(255, 255, 255);
		if (ginaManager::Get()->ginaVersion == GINA_VER_XP)
		{
			brdColor = RGB(90, 124, 223);
		}
		HBRUSH hBrush = CreateSolidBrush(brdColor);
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		rect.bottom = origBottom;
		rect.top = GINA_SMALL_BRD_HEIGHT + GINA_BAR_HEIGHT;
		COLORREF btnFace;
		btnFace = GetSysColor(COLOR_BTNFACE);
		hBrush = CreateSolidBrush(btnFace);
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		return 1;
		break;
	}
	case WM_CLOSE:
	{
		EndDialog(hWnd, 0);
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0); // Trigger exit thread
		break;
	}
	}
	return 0;
}

ginaLogoffView* ginaLogoffView::Get()
{
	static ginaLogoffView dlg;
	return &dlg;
}

void ginaLogoffView::Create(HWND parent)
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaLogoffView::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_LOGOFF), parent, (DLGPROC)DlgProc, 0);
	if (!ginaLogoffView::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create logoff dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		return;
	}
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaLogoffView::Get()->hDlg);
	}
}

void ginaLogoffView::Destroy()
{
	ginaLogoffView* dlg = ginaLogoffView::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaLogoffView::Show()
{
	ginaLogoffView* dlg = ginaLogoffView::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
	UpdateWindow(dlg->hDlg);
}

void ginaLogoffView::Hide()
{
	ginaLogoffView* dlg = ginaLogoffView::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaLogoffView::BeginMessageLoop()
{
	ginaLogoffView* dlg = ginaLogoffView::Get();
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_KEYDOWN)
		{
			switch (msg.wParam)
			{
			case VK_RETURN:
			case VK_SPACE:
			{
				TabSpace(dlg->hDlg, logoffTabIndex, sizeof(logoffTabIndex) / sizeof(logoffTabIndex[0]));
				break;
			}
			case VK_ESCAPE:
			{
				SendMessage(dlg->hDlg, WM_COMMAND, MAKEWPARAM(IDC_CANCEL, BN_CLICKED), 0);
				break;
			}
			case VK_TAB:
			{
				if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
				{
					TabPrev(dlg->hDlg, logoffTabIndex, sizeof(logoffTabIndex) / sizeof(logoffTabIndex[0]), IDC_OK);
				}
				else
				{
					TabNext(dlg->hDlg, logoffTabIndex, sizeof(logoffTabIndex) / sizeof(logoffTabIndex[0]), IDC_OK);
				}
				break;
			}
			}
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

int CALLBACK ginaLogoffView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
		HWND hLogoffIcon = GetDlgItem(hWnd, IDC_LOGOFF_ICON);
		SendMessageW(hLogoffIcon, STM_SETICON, (WPARAM)LoadIconW(hGinaDll, MAKEINTRESOURCEW(IDI_LOGOFF)), 0);
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_OK)
		{
			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_ESCAPE; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
			ExitWindowsEx(EWX_LOGOFF, 0);
			ginaLogoffView::Destroy();
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
		{
			ginaLogoffView::Destroy();
		}
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0); // Trigger exit thread
		break;
	}
	}
	return 0;
}