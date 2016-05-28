// DlgCalibration.h

// Declares the DlgCalibration class representing the calibration dialog





#pragma once





#include "Wiimote.h"
#include "Calibration.h"





class DlgCalibration
{
public:
	DlgCalibration(HINSTANCE a_Instance, WiimotePtrs a_Wiimotes);
	~DlgCalibration();

	/** Shows the dialog and waits for the user to explicitly close it.
	Returns true if the user clicks the OK button (and there is at least one fully calibrated Wiimote).
	Stores the calibration in the a_Calibration pointer given. */
	bool show(CalibrationPtr a_Calibration);

protected:

	/** The HINSTANCE of this application (where to get resources). */
	HINSTANCE m_Instance;

	/** The wiimotes that have been detected. */
	WiimotePtrs m_Wiimotes;

	/** The index of the screen (into m_Screens) on which the dialog is currently displayed. */
	int m_CurrentScreenIdx;

	/** Positions and sizes of all screens currently attached to the system. */
	std::vector<RECT> m_Screens;

	/** OS handle to the dialog window. */
	HWND m_Wnd;

	/** Index of the current calibration point, 0 .. 3 from top-left clockwise. */
	int m_CurrentCalibrationPoint;

	/** Callback for the Wiimote. Stored so that it may be removed by-reference. */
	Wiimote::Callback m_Callback;

	/** Remembered last known states of the wiimotes. Used to detect changes in state in the Wiimote callback. 
	Use getOldWiimoteState() to retrieve a (writable) state. */
	std::map<Wiimote *, Wiimote::State> m_OldWiimoteStates;

	/** The calibration data. */
	CalibrationPtr m_Calibration;


	/** The dialog box procedure, as used by WinAPI. */
	static INT_PTR CALLBACK dlgProcStatic(HWND a_Wnd, UINT a_Msg, WPARAM wParam, LPARAM lParam);

	/** The dialog box procedure, as used by WinAPI, redirected to this object instance. */
	INT_PTR dlgProc(HWND a_Wnd, UINT a_Msg, WPARAM wParam, LPARAM lParam);

	INT_PTR onInitDialog(HWND a_Wnd, WPARAM wParam, LPARAM lParam);
	INT_PTR onCommand   (HWND a_Wnd, WPARAM wParam, LPARAM lParam);

	/** Moves the dialog to the next screen (or first if all screens have been cycled). */
	void goToNextScreen();

	/** Queries all screens in the system, fills in the m_Screens member. */
	void initializeScreens();

	/** WinAPI callback used for EnumDisplayMonitors in initializeScreens(). */
	static BOOL CALLBACK enumDisplayMonitorsCallback(HMONITOR a_Monitor, HDC a_dc, LPRECT a_Rect, LPARAM a_CallbackData);

	/** (Re)sets the calibration state to the specified point.
	Updates the visual display of the point crosshair. */
	void setCurrentCalibrationPoint(int a_CurrentPoint);

	/** Callback from the Wiimote. */
	void wiimoteCallback(Wiimote & a_Wiimote);

	/** Returns the last known state of the Wiimote.
	If there's no previously known state, creates a new empty one and returns that. */
	Wiimote::State & getOldWiimoteState(Wiimote & a_Wiimote);

	/** Returns the virtual screen coordinates for the specified calibration point. */
	POINT getCalibrationPointScreenCoords(int a_CalibrationPointIndex);

	/** Returns the in-dialog coords of the crosshair center for the specified calibration point. */
	POINT getCrosshairPosForCalibrationPoint(int a_CalibrationPointIndex);

	/** Adds the m_Callback hook to all Wiimotes in m_Wiimotes. */
	void hookWiimotes();

	/** Removes the m_Callback hook from all Wiimotes in m_Wiimotes. */
	void unhookWiimotes();
};




