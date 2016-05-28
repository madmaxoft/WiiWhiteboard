// DlgViewRawData.h

// Declares the DlgViewRawData class representing the dialog for visualising the raw data received from the Wiimotes





#pragma once





#include "Wiimote.h"





class DlgViewRawData
{
public:
	DlgViewRawData(HINSTANCE a_Instance, WiimotePtrs a_Wiimotes);
	~DlgViewRawData();

	/** Shows the dialog and waits for the user to close it.
	a_ParentWnd is the OS handle to the window that should become the parent of this dialog. */
	void show(HWND a_ParentWnd);

protected:
	/** The HINSTANCE of this application (where to get resources). */
	HINSTANCE m_Instance;

	/** The wiimotes that have been detected. */
	WiimotePtrs m_Wiimotes;

	/** OS handle to the dialog window. */
	HWND m_Wnd;

	/** The callback that is added to Wiimotes so that they work with this dialog. */
	Wiimote::Callback m_Callback;

	std::map<const Wiimote *, Wiimote::IRState> m_WiimoteIRStates;


	/** The dialog box procedure, as used by WinAPI. */
	static INT_PTR CALLBACK dlgProcStatic(HWND a_Wnd, UINT a_Msg, WPARAM wParam, LPARAM lParam);

	/** The dialog box procedure, as used by WinAPI, redirected to this object instance. */
	INT_PTR dlgProc(HWND a_Wnd, UINT a_Msg, WPARAM wParam, LPARAM lParam);

	INT_PTR onInitDialog(HWND a_Wnd, WPARAM wParam, LPARAM lParam);
	INT_PTR onCommand   (HWND a_Wnd, WPARAM wParam, LPARAM lParam);
	INT_PTR onEraseBkgnd(HWND a_Wnd, WPARAM wParam, LPARAM lParam);

	/** If a_IsPresent is true, paints the dot at specified coords scaled to a_WindowRect.
	a_X, a_Y are the dot's coords, as reported by the Wiimote.
	a_WindowRect is the rectangle into which the coords are scaled
	a_DC is the HDC into which the dots are painted. */
	void paintDot(bool a_IsPresent, int a_X, int a_Y, RECT * a_WindowRect, HDC a_DC);

	/** Callback from the Wiimote when it detects a change in its state. */
	void wiimoteCallback(Wiimote & a_Wiimote);

	/** Adds the m_Callback hook to all Wiimotes in m_Wiimotes. */
	void hookWiimotes();

	/** Removes the m_Callback hook from all Wiimotes in m_Wiimotes. */
	void unhookWiimotes();
};




