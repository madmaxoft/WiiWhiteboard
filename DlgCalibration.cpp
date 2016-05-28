// DlgCalibration.cpp

// Implements the DlgCalibration class representing the calibration dialog





#include "Globals.h"
#include "DlgCalibration.h"
#include "resource.h"
#include "DlgViewRawData.h"





DlgCalibration::DlgCalibration(HINSTANCE a_Instance, WiimotePtrs a_Wiimotes):
	m_Instance(a_Instance),
	m_Wiimotes(std::move(a_Wiimotes))
{
	m_Callback = [this](Wiimote & a_Wiimote)
	{
		this->wiimoteCallback(a_Wiimote);
	};
}





DlgCalibration::~DlgCalibration()
{
}





bool DlgCalibration::show(CalibrationPtr a_Calibration)
{
	m_Calibration = a_Calibration;
	initializeScreens();
	auto res = DialogBoxParam(m_Instance, MAKEINTRESOURCE(IDD_CALIBRATION), GetDesktopWindow(), &DlgCalibration::dlgProcStatic, reinterpret_cast<LPARAM>(this));
	unhookWiimotes();
	return (res == IDOK);
}





INT_PTR CALLBACK DlgCalibration::dlgProcStatic(HWND a_Wnd, UINT a_Msg, WPARAM wParam, LPARAM lParam)
{
	// The WM_INITDIALOG message is the first one sent, its lParam is the DlgCalibration * instance; store it:
	if (a_Msg == WM_INITDIALOG)
	{
		SetWindowLongPtr(a_Wnd, GWLP_USERDATA, lParam);
		return reinterpret_cast<DlgCalibration *>(lParam)->onInitDialog(a_Wnd, wParam, lParam);
	}

	// Get the instance and call its dlgProc:
	auto dlg = reinterpret_cast<DlgCalibration *>(GetWindowLongPtr(a_Wnd, GWLP_USERDATA));
	if (dlg == nullptr)
	{
		return FALSE;
	}
	return dlg->dlgProc(a_Wnd, a_Msg, wParam, lParam);
}





INT_PTR DlgCalibration::dlgProc(HWND a_Wnd, UINT a_Msg, WPARAM wParam, LPARAM lParam)
{
	switch (a_Msg)
	{
		case WM_COMMAND: return onCommand(a_Wnd, wParam, lParam);
	}
	return FALSE;
}





INT_PTR DlgCalibration::onInitDialog(HWND a_Wnd, WPARAM wParam, LPARAM lParam)
{
	m_Wnd = a_Wnd;
	m_CurrentScreenIdx = -1;
	goToNextScreen();
	hookWiimotes();
	return FALSE;
}





INT_PTR DlgCalibration::onCommand(HWND a_Wnd, WPARAM wParam, LPARAM lParam)
{
	auto command = LOWORD(wParam);
	auto action = HIWORD(wParam);
	switch (command)
	{
		case IDOK:
		{
			if (action == BN_CLICKED)
			{
				EndDialog(a_Wnd, IDOK);
				return TRUE;
			}
			break;
		}
		case IDCANCEL:
		{
			if (action == BN_CLICKED)
			{
				EndDialog(a_Wnd, IDCANCEL);
				return TRUE;
			}
			break;
		}
		case IDC_B_SKIPSCREEN:
		{
			if (action == BN_CLICKED)
			{
				goToNextScreen();
				return TRUE;
			}
			break;
		}
		case IDC_B_VIEWRAW:
		{
			if (action == BN_CLICKED)
			{
				unhookWiimotes();
				DlgViewRawData dlg(m_Instance, m_Wiimotes);
				dlg.show(a_Wnd);
				hookWiimotes();
			}
			break;
		}
	}
	return FALSE;
}





void DlgCalibration::goToNextScreen()
{
	m_CurrentScreenIdx = (m_CurrentScreenIdx + 1) % m_Screens.size();
	auto & currentScreen = m_Screens[m_CurrentScreenIdx];
	MoveWindow(m_Wnd, currentScreen.left, currentScreen.top, currentScreen.right - currentScreen.left, currentScreen.bottom - currentScreen.top, TRUE);
	setCurrentCalibrationPoint(0);
}





void DlgCalibration::initializeScreens()
{
	m_Screens.clear();
	EnumDisplayMonitors(nullptr, nullptr, &DlgCalibration::enumDisplayMonitorsCallback, reinterpret_cast<LPARAM>(this));
}





BOOL CALLBACK DlgCalibration::enumDisplayMonitorsCallback(HMONITOR a_Monitor, HDC a_dc, LPRECT a_Rect, LPARAM a_CallbackData)
{
	UNUSED(a_Monitor);
	UNUSED(a_dc);

	auto dlg = reinterpret_cast<DlgCalibration *>(a_CallbackData);
	assert(dlg != nullptr);
	dlg->m_Screens.push_back(*a_Rect);
	return TRUE;
}





void DlgCalibration::setCurrentCalibrationPoint(int a_CalibrationPoint)
{
	m_CurrentCalibrationPoint = a_CalibrationPoint;
	auto icoCrosshair = GetDlgItem(m_Wnd, IDC_ICO_CROSSHAIR);
	RECT r;
	GetWindowRect(icoCrosshair, &r);
	auto wid = r.right - r.left;
	auto hei = r.bottom - r.top;
	auto center = getCrosshairPosForCalibrationPoint(a_CalibrationPoint);
	MoveWindow(icoCrosshair, center.x - wid / 2, center.y - hei / 2, wid, hei, TRUE);
	UpdateWindow(icoCrosshair);
	InvalidateRect(icoCrosshair, nullptr, TRUE);
}





void DlgCalibration::wiimoteCallback(Wiimote & a_Wiimote)
{
	auto & oldState = getOldWiimoteState(a_Wiimote);
	auto irState = a_Wiimote.getCurrentIRState();
	if (!oldState.m_IRState.m_IsPresent1 && irState.m_IsPresent1)
	{
		// "Mouse click", remember the pos for calibration, normalize to primary screen ( https://msdn.microsoft.com/en-us/library/windows/desktop/ms646273%28v=vs.85%29.aspx ; Remarks section):
		auto screenCoords = getCalibrationPointScreenCoords(m_CurrentCalibrationPoint);
		m_Calibration->setPoint(a_Wiimote, m_CurrentCalibrationPoint, irState.m_X1, irState.m_Y1, screenCoords.x * 65535 / m_Screens[0].right, screenCoords.y * 65535 / m_Screens[0].bottom);
		if (m_CurrentCalibrationPoint == 3)
		{
			EnableWindow(GetDlgItem(m_Wnd, IDOK), m_Calibration->isUsable() ? TRUE : FALSE);
			goToNextScreen();
		}
		else
		{
			setCurrentCalibrationPoint(m_CurrentCalibrationPoint + 1);
		}
	}
	oldState.m_IRState = irState;
}





Wiimote::State & DlgCalibration::getOldWiimoteState(Wiimote & a_Wiimote)
{
	auto itr = m_OldWiimoteStates.find(&a_Wiimote);
	if (itr == m_OldWiimoteStates.end())
	{
		// No such state, create a new empty one:
		Wiimote::State state{};
		itr = m_OldWiimoteStates.insert(m_OldWiimoteStates.begin(), std::make_pair(&a_Wiimote, state));
	}
	return itr->second;
}





POINT DlgCalibration::getCalibrationPointScreenCoords(int a_CalibrationPointIndex)
{
	POINT p = getCrosshairPosForCalibrationPoint(a_CalibrationPointIndex);
	ClientToScreen(m_Wnd, &p);
	return p;
}





POINT DlgCalibration::getCrosshairPosForCalibrationPoint(int a_CalibrationPointIndex)
{
	// Get the crosshair dimensions:
	auto icoCrosshair = GetDlgItem(m_Wnd, IDC_ICO_CROSSHAIR);
	RECT r;
	GetWindowRect(icoCrosshair, &r);
	auto wid = r.right - r.left;
	auto hei = r.bottom - r.top;

	// Get the current dialog dimensions:
	RECT curWindow;
	GetClientRect(m_Wnd, &curWindow);
	auto right = curWindow.right - 10 - wid / 2;
	auto left = 10 + wid / 2;
	auto bottom = curWindow.bottom - 10 - hei / 2;
	auto top = 10 + hei / 2;

	// Calculate the crosshair pos:
	switch (a_CalibrationPointIndex)
	{
		case 0: return {left,  top};
		case 1: return {right, top};
		case 2: return {right, bottom};
		case 3: return {left,  bottom};
	}
	assert(!"Wrong CalibrationPoint");
	return {0, 0};
}





void DlgCalibration::hookWiimotes()
{
	for (auto & wiimote: m_Wiimotes)
	{
		wiimote->addCallback(&m_Callback);
	}
}





void DlgCalibration::unhookWiimotes()
{
	for (auto & wiimote: m_Wiimotes)
	{
		wiimote->removeCallback(&m_Callback);
	}
}




