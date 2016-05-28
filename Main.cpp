// Main.cpp

// Implements the main app entrypoint





#include "Globals.h"
#include "WiimoteManager.h"
#include "DlgCalibration.h"
#include "Warper.h"
#include "Processor.h"






int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Find out the connected wiimotes:
	LOG("Detecting Wiimotes...");
	auto & mgr = WiimoteManager::get();
	auto ids = mgr.enumWiimotes();
	if (ids.empty())
	{
		MessageBox(nullptr, TEXT("There is no Wiimote currently attached to the computer"), TEXT("WiiWhiteboard Error"), MB_OK);
		return 1;
	}

	// Start each Wiimote:
	LOG("Starting Wiimotes...");
	int i = 1;
	WiimotePtrs wiimotes;
	for (const auto & id: ids)
	{
		auto wiimote = std::make_shared<Wiimote>();
		if (!wiimote->connect(id, nullptr))
		{
			continue;
		}
		wiimote->setLeds(((i & 0x01) != 0), ((i & 0x02) != 0), ((i & 0x04) != 0), ((i & 0x08) != 0));
		wiimote->setReportType(Wiimote::irtIRAccel, true);
		wiimotes.push_back(wiimote);
		i = i + 1;
	}
	LOG("%u wiimotes have been successfully connected", static_cast<unsigned>(wiimotes.size()));
	if (wiimotes.empty())
	{
		MessageBox(nullptr, TEXT("Cannot connect to any Wiimote.\r\n\r\nYou need to remove any Wiimotes from the Devices and Printer and then re-add them from there."), TEXT("WiiWhiteboard Error"), MB_OK);
		return 2;
	}

	// Calibrate:
	LOG("Displaying the Calibration UI...");
	CalibrationPtr calibration = std::make_shared<Calibration>();
	DlgCalibration d(hInstance, wiimotes);
	if (!d.show(calibration))
	{
		LOG("Calibration cancelled, exitting");
		return 3;
	}

	// Set up the warper and callbacks:
	Warper warper;
	warper.setCalibration(*calibration);
	std::vector<ProcessorPtr> processors;
	for (const auto w: warper.getWarpableWiimotes())
	{
		processors.push_back(std::make_shared<Processor>(warper, wiimotes, w));
	}

	// Lurk in the background and emulate mouse
	LOG("Running...");
	HWND mainWnd = CreateWindow(TEXT("STATIC"), TEXT("STATIC"), WS_POPUPWINDOW, 0, 0, 0, 0, nullptr, nullptr, hInstance, 0);
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (IsDialogMessage(mainWnd, &msg))
		{
			continue;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	DestroyWindow(mainWnd);
	return 0;
}




