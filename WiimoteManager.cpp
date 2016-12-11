// WiimoteManager.cpp

// Implements the WiimoteManager class representing the singleton that manages individual Wiimote instances





#include "Globals.h"
#include <hidclass.h>
#include <hidsdi.h>
#include <SetupAPI.h>
#include "WiimoteManager.h"
#include "HandleGuard.h"





WiimoteManager & WiimoteManager::get()
{
	static WiimoteManager singleton;
	return singleton;
}





WiimoteManager::WiimoteManager()
{
}





WiimoteManager::~WiimoteManager()
{
}





Wiimote::Ids WiimoteManager::enumWiimotes()
{
	// Enumerate all devices:
	GUID hidGuid;
	HidD_GetHidGuid(&hidGuid);
	auto hDevInfo = SetupDiGetClassDevs(&hidGuid, nullptr, nullptr, DIGCF_DEVICEINTERFACE);
	SP_DEVICE_INTERFACE_DATA diData;
	diData.cbSize = sizeof(diData);
	Wiimote::Ids res;
	for (DWORD index = 0; SetupDiEnumDeviceInterfaces(hDevInfo, nullptr, &hidGuid, index, &diData); ++index)
	{
		// Get the device path:
		DWORD size;
		char buffer[4000];  // The fixed-size back-buffer for diDetail
		SetupDiGetDeviceInterfaceDetailW(hDevInfo, &diData, nullptr, 0, &size, nullptr);
		if (size > sizeof(buffer))
		{
			OutputDebugStringA("Device interface detail size too large for the fixed-size buffer\n");
			continue;
		}
		auto diDetail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(&buffer);
		diDetail->cbSize = sizeof(*diDetail);
		if (!SetupDiGetDeviceInterfaceDetailW(hDevInfo, &diData, diDetail, size, nullptr, nullptr))
		{
			LOG("SetupDiGetDeviceInterfaceDetailW(#%d) failed: %d", index, GetLastError());
			continue;
		}

		// Check the VID and PID identifiers:
		HIDD_ATTRIBUTES attrib;
		attrib.Size = sizeof(attrib);
		HandleGuard handle(CreateFileW(diDetail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
		if (!HidD_GetAttributes(handle, &attrib))
		{
			LOG("Unavailable HID Device: path \"%s\".", Wiimote::IdFromWPath(diDetail->DevicePath).c_str());
			continue;
		}
		LOG("HID device: VID 0x%04x, PID 0x%04x, path \"%s\"", attrib.VendorID, attrib.ProductID, Wiimote::IdFromWPath(diDetail->DevicePath).c_str());
		if ((attrib.VendorID != Wiimote::VendorID) || (attrib.ProductID != Wiimote::ProductID))
		{
			continue;
		}
		LOG("  ^^ This is a Wiimote");
		res.push_back(Wiimote::IdFromWPath(diDetail->DevicePath));
	}

	return res;
}




