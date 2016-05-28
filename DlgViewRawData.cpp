// DlgViewRawData.cpp

// Implements the DlgViewRawData class representing the dialog for visualising the raw data received from the Wiimotes





#include "Globals.h"
#include "DlgViewRawData.h"
#include "resource.h"





DlgViewRawData::DlgViewRawData(HINSTANCE a_Instance, WiimotePtrs a_Wiimotes):
	m_Instance(a_Instance),
	m_Wiimotes(a_Wiimotes),
	m_Wnd(nullptr)
{
	m_Callback = [this](Wiimote & a_Wiimote)
	{
		this->wiimoteCallback(a_Wiimote);
	};
}





DlgViewRawData::~DlgViewRawData()
{
	unhookWiimotes();
}





void DlgViewRawData::show(HWND a_ParentWnd)
{
	DialogBoxParam(m_Instance, MAKEINTRESOURCE(IDD_RAWDATA), a_ParentWnd, &DlgViewRawData::dlgProcStatic, reinterpret_cast<LPARAM>(this));
}





INT_PTR CALLBACK DlgViewRawData::dlgProcStatic(HWND a_Wnd, UINT a_Msg, WPARAM wParam, LPARAM lParam)
{
	// The WM_INITDIALOG message is the first one sent, its lParam is the DlgViewRawData * instance; store it:
	if (a_Msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(a_Wnd, GWLP_USERDATA, lParam);
		return reinterpret_cast<DlgViewRawData *>(lParam)->onInitDialog(a_Wnd, wParam, lParam);
	}

	// Get the instance and call its dlgProc:
	auto dlg = reinterpret_cast<DlgViewRawData *>(GetWindowLongPtr(a_Wnd, GWLP_USERDATA));
	if (dlg == nullptr)
	{
		return FALSE;
	}
	return dlg->dlgProc(a_Wnd, a_Msg, wParam, lParam);
}





INT_PTR DlgViewRawData::dlgProc(HWND a_Wnd, UINT a_Msg, WPARAM wParam, LPARAM lParam)
{
	switch (a_Msg)
	{
		case WM_COMMAND:    return onCommand(a_Wnd, wParam, lParam);
		case WM_ERASEBKGND: return onEraseBkgnd(a_Wnd, wParam, lParam);
	}
	return FALSE;
}





INT_PTR DlgViewRawData::onInitDialog(HWND a_Wnd, WPARAM wParam, LPARAM lParam)
{
	m_Wnd = a_Wnd;
	hookWiimotes();
	return FALSE;
}





INT_PTR DlgViewRawData::onCommand(HWND a_Wnd, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDOK:
		{
			if (HIWORD(wParam) == BN_CLICKED)
			{
				EndDialog(a_Wnd, IDOK);
				return TRUE;
			}
		}
	}
	return FALSE;
}





INT_PTR DlgViewRawData::onEraseBkgnd(HWND a_Wnd, WPARAM wParam, LPARAM lParam)
{
	auto br = GetSysColorBrush(COLOR_3DFACE);
	auto dc = reinterpret_cast<HDC>(wParam);
	RECT rc;
	GetClientRect(a_Wnd, &rc);
	FillRect(dc, &rc, br);
	for (const auto s: m_WiimoteIRStates)
	{
		paintDot(s.second.m_IsPresent1, s.second.m_X1, s.second.m_Y1, &rc, dc);
		paintDot(s.second.m_IsPresent2, s.second.m_X2, s.second.m_Y2, &rc, dc);
		paintDot(s.second.m_IsPresent3, s.second.m_X3, s.second.m_Y3, &rc, dc);
		paintDot(s.second.m_IsPresent4, s.second.m_X4, s.second.m_Y4, &rc, dc);
	}
	return TRUE;
}





void DlgViewRawData::paintDot(bool a_IsPresent, int a_X, int a_Y, RECT * a_WindowRect, HDC a_DC)
{
	if (!a_IsPresent)
	{
		return;
	}
	int x = a_WindowRect->left + a_X * (a_WindowRect->right - a_WindowRect->left) / 1024;
	int y = a_WindowRect->top  + a_Y * (a_WindowRect->bottom - a_WindowRect->top) / 768;
	Ellipse(a_DC, x - 3, y - 3, x + 4, y + 4);
}





void DlgViewRawData::wiimoteCallback(Wiimote & a_Wiimote)
{
	m_WiimoteIRStates[&a_Wiimote] = a_Wiimote.getCurrentIRState();
	InvalidateRect(m_Wnd, nullptr, TRUE);
}





void DlgViewRawData::hookWiimotes()
{
	for (auto & wiimote: m_Wiimotes)
	{
		wiimote->addCallback(&m_Callback);
	}
}





void DlgViewRawData::unhookWiimotes()
{
	for (auto & wiimote: m_Wiimotes)
	{
		wiimote->removeCallback(&m_Callback);
	}
}




