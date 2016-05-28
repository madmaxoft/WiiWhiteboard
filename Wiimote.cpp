// Wiimote.h

// Implements the Wiimote class representing a single connected Wiimote controller





#include "Globals.h"
#include "Wiimote.h"
#include <hidsdi.h>





/** The size of the input and output reports. */
static const size_t REPORT_SIZE = 22;

// Memory-mapped registers in the Wiimote:
static const int REGISTER_IR = 0x04b00030;
static const int REGISTER_IR_SENSITIVITY_1 = 0x04b00000;
static const int REGISTER_IR_SENSITIVITY_2 = 0x04b0001a;
static const int REGISTER_IR_MODE = 0x04b00033;

/** The output report type, sent to the Wiimote. */
enum
{
	ortNone        = 0x00,
	ortLEDs        = 0x11,
	ortType        = 0x12,
	ortIR          = 0x13,
	ortStatus      = 0x15,
	ortWriteMemory = 0x16,
	ortReadMemory  = 0x17,
	ortIR2         = 0x1a,
};





Wiimote::Wiimote():
	m_Handle(INVALID_HANDLE_VALUE),
	m_ShouldTerminate(false),
	m_CurrentState(),
	m_UseAltWrite(false),
	m_WriteEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)),
	m_LastWriteTime(std::chrono::system_clock::now())
{
}





Wiimote::~Wiimote()
{
	// Notify the Reader thread to terminate, and wait for it to do so:
	if (m_Handle != INVALID_HANDLE_VALUE)
	{
		m_ShouldTerminate = true;
		CancelIoEx(m_Handle, nullptr);
		CloseHandle(m_Handle);
		m_ReadThread.join();
	}
}





bool Wiimote::connect(const Wiimote::Id & a_Id, Wiimote::Callback * a_InitialCallback)
{
	assert(m_Handle == INVALID_HANDLE_VALUE);  // Not connected yet

	// Open the OS handle:
	auto path = WPathFromId(a_Id);
	// Need to use OVERLAPPED IO, because in non-OVERLAPPED we get deadlocked by the OS if the Wiimote breaks the bluetooth connection
	m_Handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		auto gle = GetLastError();
		LOG("Wiimote \"%s\": failed to open OS handle: %d (0x%x)", a_Id.c_str(), gle, gle);
		return false;
	}

	// Insert the initial callback into the list of callbacks:
	if (a_InitialCallback != nullptr)
	{
		m_Callbacks.push_back(a_InitialCallback);
	}

	// Start the async reading:
	m_ReadThread = std::thread(&Wiimote::thrRead, this);

	// Try to read the calibration data:
	auto isSuccess = readCalibration();
	if (!isSuccess)
	{
		m_UseAltWrite = true;  // Use an alternate write method
		isSuccess = readCalibration();
		if (!isSuccess)
		{
			LOG("Wiimote \"%s\": failed to read calibration data", a_Id.c_str());
			m_ShouldTerminate = true;
			CancelIoEx(m_Handle, nullptr);
			CloseHandle(m_Handle);
			m_Handle = INVALID_HANDLE_VALUE;
			m_ReadThread.join();
			return false;
		}
	}

	return true;
}





Wiimote::Id Wiimote::IdFromWPath(LPCWSTR a_DevicePath)
{
	char pathUtf8[12000];
	WideCharToMultiByte(CP_UTF8, 0, a_DevicePath, -1, pathUtf8, ARRAYCOUNT(pathUtf8), nullptr, nullptr);
	return pathUtf8;
}





std::wstring Wiimote::WPathFromId(const Id & a_Id)
{
	WCHAR pathUtf16[12000];
	auto res = MultiByteToWideChar(CP_UTF8, 0, a_Id.c_str(), static_cast<int>(a_Id.size()), pathUtf16, ARRAYCOUNT(pathUtf16));
	if (res >= 0)
	{
		pathUtf16[res] = 0;
	}
	return pathUtf16;
}





Wiimote::IRState Wiimote::getCurrentIRState() const
{
	std::lock_guard<std::mutex> lock(m_CS);
	return m_CurrentState.m_IRState;
}





void Wiimote::setLeds(bool a_Led1, bool a_Led2, bool a_Led3, bool a_Led4)
{
	std::lock_guard<std::mutex> lock (m_CS);
	m_CurrentState.m_Led1 = a_Led1;
	m_CurrentState.m_Led2 = a_Led2;
	m_CurrentState.m_Led3 = a_Led3;
	m_CurrentState.m_Led4 = a_Led4;

	unsigned char req[2] =
	{
		ortLEDs,
		static_cast<unsigned char>(
			(a_Led1 ? 0x10 : 0x00) |
			(a_Led2 ? 0x20 : 0x00) |
			(a_Led3 ? 0x40 : 0x00) |
			(a_Led4 ? 0x80 : 0x00) |
			getRumbleBit()
		)
	};
	writeReport(req, 2);
}





void Wiimote::setReportType(InputReportType a_Type, bool a_Continuous)
{
	switch (a_Type)
	{
		case irtIRAccel:
		{
			enableIR(irrmExtended);
			break;
		}
		case irtIRExtensionAccel:
		{
			enableIR(irrmBasic);
			break;
		}
		default:
		{
			disableIR();
			break;
		}
	}

	unsigned char req[3] =
	{
		ortType,
		static_cast<unsigned char>((a_Continuous ? 0x04 : 0x00) | getRumbleBit()),
		static_cast<unsigned char>(a_Type),
	};
	writeReport(req, 3);
}





void Wiimote::addCallback(Callback * a_Callback)
{
	std::lock_guard<std::mutex> lock (m_CS);
	m_Callbacks.push_back(a_Callback);
}





void Wiimote::removeCallback(Callback * a_Callback)
{
	std::lock_guard<std::mutex> lock (m_CS);
	m_Callbacks.remove(a_Callback);
}





void Wiimote::enableIR(IRReportingMode a_Mode)
{
	unsigned char rumbleBit;
	{
		std::lock_guard<std::mutex> lock(m_CS);
		rumbleBit = getRumbleBit();
	}

	char req1[2] =
	{
		ortIR,
		0x04 | rumbleBit,
	};
	writeReport(req1, 2);

	char req2[2] =
	{
		ortIR2,
		0x04 | rumbleBit,
	};
	writeReport(req2, 2);

	writeData(REGISTER_IR, 0x08);

	// Super sensitive, according to http://wiibrew.org/index.php?title=Wiimote#Sensitivity_Settings
	unsigned char sensitivity1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x41 };
	unsigned char sensitivity2[] = { 0x40, 0x00 };
	writeData(REGISTER_IR_SENSITIVITY_1, sensitivity1, sizeof(sensitivity1));
	writeData(REGISTER_IR_SENSITIVITY_2, sensitivity2, sizeof(sensitivity2));

	writeData(REGISTER_IR_MODE, static_cast<unsigned char>(a_Mode));

	// Store the new mode, so that the parser can use it:
	std::lock_guard<std::mutex> lock(m_CS);
	m_CurrentState.m_IRState.m_ReportingMode = a_Mode;
}





void Wiimote::disableIR()
{
	unsigned char rumbleBit;
	{
		std::lock_guard<std::mutex> lock(m_CS);
		m_CurrentState.m_IRState.m_ReportingMode = irrmOff;
		rumbleBit = getRumbleBit();
	}

	unsigned char req[2] =
	{
		ortIR,
		rumbleBit,
	};
	writeReport(req, 2);

	req[0] = ortIR2;
	writeReport(req, 2);
}





bool Wiimote::parseIncomingPacket(const unsigned char * a_Packet)
{
	std::lock_guard<std::mutex> lock(m_CS);
	auto reportType = a_Packet[0];
	switch (reportType)
	{
		case irtButtons:
		{
			parseButtons(a_Packet);
			break;
		}
		case irtButtonsAccel:
		{
			parseButtons(a_Packet);
			parseAccel(a_Packet);
			break;
		}
		case irtButtonsExtension:
		{
			break;
		}
		case irtExtensionAccel:
		{
			break;
		}
		case irtIRAccel:
		{
			parseButtons(a_Packet);
			parseAccel(a_Packet);
			parseIR(a_Packet);
			break;
		}
		case irtIRExtensionAccel:
		{
			break;
		}
		case irtReadData:
		{
			parseButtons(a_Packet);
			parseReadData(a_Packet);
			break;
		}
		case irtStatus:
		{
			break;
		}
		default:
		{
			// Unknown report
			return false;
		}
	}
	return true;
}





void Wiimote::parseAccel(const unsigned char * a_Packet)
{
	m_CurrentState.m_AccelState.m_AccelX = static_cast<unsigned char>(a_Packet[3]) | ((a_Packet[1] & 0x60) << 8);
	m_CurrentState.m_AccelState.m_AccelY = static_cast<unsigned char>(a_Packet[4]) | ((a_Packet[2] & 0x20) << 8);
	m_CurrentState.m_AccelState.m_AccelZ = static_cast<unsigned char>(a_Packet[5]) | ((a_Packet[2] & 0x40) << 8);
}





void Wiimote::parseButtons(const unsigned char * a_Packet)
{
	m_CurrentState.m_ButtonState.m_ButtonA     = (a_Packet[2] & 0x08) != 0;
	m_CurrentState.m_ButtonState.m_ButtonB     = (a_Packet[2] & 0x04) != 0;
	m_CurrentState.m_ButtonState.m_ButtonMinus = (a_Packet[2] & 0x10) != 0;
	m_CurrentState.m_ButtonState.m_ButtonHome  = (a_Packet[2] & 0x80) != 0;
	m_CurrentState.m_ButtonState.m_ButtonPlus  = (a_Packet[1] & 0x10) != 0;
	m_CurrentState.m_ButtonState.m_ButtonOne   = (a_Packet[2] & 0x02) != 0;
	m_CurrentState.m_ButtonState.m_ButtonTwo   = (a_Packet[2] & 0x01) != 0;
	m_CurrentState.m_ButtonState.m_ButtonUp    = (a_Packet[1] & 0x08) != 0;
	m_CurrentState.m_ButtonState.m_ButtonDown  = (a_Packet[1] & 0x04) != 0;
	m_CurrentState.m_ButtonState.m_ButtonLeft  = (a_Packet[1] & 0x01) != 0;
	m_CurrentState.m_ButtonState.m_ButtonRight = (a_Packet[1] & 0x02) != 0;
}





void Wiimote::parseIR(const unsigned char * a_Packet)
{
	m_CurrentState.m_IRState.m_X1 = static_cast<int>(a_Packet[6])  | (((a_Packet[8] >> 4) & 0x03) << 8);
	m_CurrentState.m_IRState.m_Y1 = static_cast<int>(a_Packet[7])  | (((a_Packet[8] >> 6) & 0x03) << 8);

	switch(m_CurrentState.m_IRState.m_ReportingMode)
	{
		case irrmBasic:
		{
			m_CurrentState.m_IRState.m_X2 = a_Packet[9]  | (((a_Packet[8]  >> 0) & 0x03) << 8);
			m_CurrentState.m_IRState.m_Y2 = a_Packet[10] | (((a_Packet[8]  >> 2) & 0x03) << 8);
			m_CurrentState.m_IRState.m_X3 = a_Packet[11] | (((a_Packet[13] >> 4) & 0x03) << 8);
			m_CurrentState.m_IRState.m_Y3 = a_Packet[12] | (((a_Packet[13] >> 6) & 0x03) << 8);
			m_CurrentState.m_IRState.m_X4 = a_Packet[14] | (((a_Packet[13] >> 0) & 0x03) << 8);
			m_CurrentState.m_IRState.m_Y4 = a_Packet[15] | (((a_Packet[13] >> 2) & 0x03) << 8);
			m_CurrentState.m_IRState.m_IsPresent1 = !((a_Packet[6]  == 0xff) && (a_Packet[7]  == 0xff));
			m_CurrentState.m_IRState.m_IsPresent2 = !((a_Packet[9]  == 0xff) && (a_Packet[10] == 0xff));
			m_CurrentState.m_IRState.m_IsPresent3 = !((a_Packet[11] == 0xff) && (a_Packet[12] == 0xff));
			m_CurrentState.m_IRState.m_IsPresent4 = !((a_Packet[14] == 0xff) && (a_Packet[15] == 0xff));
			break;
		}
		case irrmExtended:
		{
			m_CurrentState.m_IRState.m_X2 = a_Packet[9]  | (((a_Packet[11] >> 4) & 0x03) << 8);
			m_CurrentState.m_IRState.m_Y2 = a_Packet[10] | (((a_Packet[11] >> 6) & 0x03) << 8);
			m_CurrentState.m_IRState.m_X3 = a_Packet[12] | (((a_Packet[14] >> 4) & 0x03) << 8);
			m_CurrentState.m_IRState.m_Y3 = a_Packet[13] | (((a_Packet[14] >> 6) & 0x03) << 8);
			m_CurrentState.m_IRState.m_X4 = a_Packet[15] | (((a_Packet[17] >> 4) & 0x03) << 8);
			m_CurrentState.m_IRState.m_Y4 = a_Packet[16] | (((a_Packet[17] >> 6) & 0x03) << 8);
			m_CurrentState.m_IRState.m_IsPresent1 = !((a_Packet[6]  == 0xff) && (a_Packet[7]  == 0xff) && (a_Packet[8]  == 0xff));
			m_CurrentState.m_IRState.m_IsPresent2 = !((a_Packet[9]  == 0xff) && (a_Packet[10] == 0xff) && (a_Packet[11] == 0xff));
			m_CurrentState.m_IRState.m_IsPresent3 = !((a_Packet[12] == 0xff) && (a_Packet[13] == 0xff) && (a_Packet[14] == 0xff));
			m_CurrentState.m_IRState.m_IsPresent4 = !((a_Packet[15] == 0xff) && (a_Packet[16] == 0xff) && (a_Packet[17] == 0xff));
			break;
		}
	}
}





void Wiimote::parseReadData(const unsigned char * a_Packet)
{
	char * buf;
	short left;
	std::condition_variable * cv;
	std::tie(buf, left, cv) = m_ReadDataRequests.front();
	if ((a_Packet[3] & 0x07) != 0)
	{
		// The ReadData requet failed device-side, remove the request from the queue:
		LOG("Request to read data has failed device-side");
		m_ReadDataRequests.pop_front();
		cv->notify_all();
	}

	// Copy the received data to the buffer:
	auto dataSize = (a_Packet[3] >> 4) + 1;
	memcpy(buf, a_Packet + 4, std::min<short>(dataSize, left));

	// If the entire read operation was satisfied, remove it from the queue:
	if (dataSize >= left)
	{
		m_ReadDataRequests.pop_front();
		cv->notify_all();
	}
	else
	{
		// More data expected, update the counter:
		std::get<1>(m_ReadDataRequests.front()) = left - dataSize;
	}
}





void Wiimote::thrRead()
{
	auto evtRead = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	while (!m_ShouldTerminate)
	{
		unsigned char buffer[22];
		DWORD br = 0;
		OVERLAPPED ovl;
		memset(&ovl, 0, sizeof(ovl));
		ovl.hEvent = evtRead;
		if (!ReadFile(m_Handle, &buffer, sizeof(buffer), &br, &ovl))
		{
			auto gle = GetLastError();
			if (gle != ERROR_IO_PENDING)
			{
				LOG("Wiimote \"%s\": ReadFile() failed: %d (0x%x)", m_Id.c_str(), gle, gle);
				return;
			}
			if (!GetOverlappedResult(m_Handle, &ovl, &br, TRUE))
			{
				auto gle = GetLastError();
				LOG("Wiimote \"%s\": Failed to wait for read data: %d (0x%x)", m_Id.c_str(), gle, gle);
				return;
			}
		}
		if (br != sizeof(buffer))
		{
			auto gle = GetLastError();
			LOG("Wiimote \"%s\": Wrong amount of data read. Exp %u, got %u.", m_Id.c_str(), static_cast<unsigned>(sizeof(buffer)), br);
			return;
		}
		if (parseIncomingPacket(buffer))
		{
			notifyStateChange();
		}
	}
}





void Wiimote::notifyStateChange()
{
	// Make a copy of the callback list:
	std::list<Callback *> callbacks;
	{
		std::lock_guard<std::mutex> lock(m_CS);
		callbacks = m_Callbacks;
	}

	// Call the callbacks from the list:
	for (auto & cb: callbacks)
	{
		try
		{
			(*cb)(*this);
		}
		catch (const std::exception & exc)
		{
			LOG("Wiimote \"%s\": Failed to call callback: %s", m_Id.c_str(), exc.what());
		}
	}
}




bool Wiimote::readCalibration()
{
	char buf[7];
	if (!readData(0x0016, 7, buf))
	{
		return false;
	}

	m_CurrentState.m_AccelCalibration.m_X0 = buf[0];
	m_CurrentState.m_AccelCalibration.m_Y0 = buf[1];
	m_CurrentState.m_AccelCalibration.m_Z0 = buf[2];
	m_CurrentState.m_AccelCalibration.m_XG = buf[4];
	m_CurrentState.m_AccelCalibration.m_YG = buf[5];
	m_CurrentState.m_AccelCalibration.m_ZG = buf[6];
	return true;
}




bool Wiimote::readData(int a_Address, short a_Size, char * a_Buffer)
{
	// Write the output report:
	std::unique_lock<std::mutex> lock(m_CS);
	unsigned char req[7] =
	{
		ortReadMemory,
		static_cast<unsigned char>(((a_Address & 0xff000000) >> 24) | getRumbleBit()),
		static_cast<unsigned char>((a_Address & 0x00ff0000)  >> 16),
		static_cast<unsigned char>((a_Address & 0x0000ff00)  >>  8),
		static_cast<unsigned char>(a_Address & 0x000000ff),
		static_cast<unsigned char>((a_Size & 0xff00) >> 8),
		static_cast<unsigned char>(a_Size & 0xff),
	};
	if (!writeReport(req, 7))
	{
		LOG("Wiimote \"%s\": readData() failed to write request", m_Id.c_str());
		return false;
	}

	// Add the read request to the queue and wait for completion:
	std::condition_variable cv;
	m_ReadDataRequests.emplace_back(a_Buffer, a_Size, &cv);
	auto status = cv.wait_for(lock, std::chrono::seconds(1));
	if (status == std::cv_status::no_timeout)
	{
		return true;
	}
	LOG("Wiimote \"%s\": readData() timed out", m_Id.c_str());
	return false;
}





bool Wiimote::writeData(int a_Address, unsigned char a_Value)
{
	return writeData(a_Address, &a_Value, 1);
}





bool Wiimote::writeData(int a_Address, const void * a_Values, unsigned char a_Size)
{
	assert(a_Size <= REPORT_SIZE - 6);
	unsigned char req[REPORT_SIZE] =
	{
		ortWriteMemory,
		static_cast<unsigned char>(((a_Address & 0xff000000) >> 24) | getRumbleBit()),
		static_cast<unsigned char>((a_Address & 0x00ff0000) >> 16),
		static_cast<unsigned char>((a_Address & 0x0000ff00) >> 8),
		static_cast<unsigned char>(a_Address & 0x000000ff),
		a_Size,
	};
	memcpy(req + 6, a_Values, std::min<size_t>(a_Size, sizeof(req) - 6));
	return writeReport(req, sizeof(req));
}





bool Wiimote::writeReport(const void * a_Buffer, size_t a_Size)
{
	std::lock_guard<std::mutex> lock(m_CSWrite);
	auto sinceLastWrite = std::chrono::system_clock::now() - m_LastWriteTime;
	if (sinceLastWrite < std::chrono::milliseconds(100))
	{
		auto toSleepTS = std::chrono::milliseconds(100) - sinceLastWrite;
		auto toSleepMS = std::chrono::duration_cast<std::chrono::milliseconds>(toSleepTS);
		auto toSleep = static_cast<DWORD>(toSleepMS.count());
		LOG("Wiimote \"%s\": Sleeping for %u milliseconds before writing", m_Id.c_str(), toSleep);
		Sleep(toSleep);
	}
	m_LastWriteTime = std::chrono::system_clock::now();

	char buf[REPORT_SIZE];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, a_Buffer, std::min(a_Size, sizeof(buf)));
	if (m_UseAltWrite)
	{
		auto res = !!HidD_SetOutputReport(m_Handle, buf, sizeof(buf));
		if (!res)
		{
			auto gle = GetLastError();
			LOG("Wiimote \"%s\": Failed to SetOutputReport: %d (0x%x)", m_Id.c_str(), gle, gle);
		}
		return res;
	}

	// Write using WriteFile:
	DWORD bw;
	OVERLAPPED ovl;
	memset(&ovl, 0, sizeof(ovl));
	ovl.hEvent = m_WriteEvent;
	if (!WriteFile(m_Handle, buf, static_cast<DWORD>(sizeof(buf)), &bw, &ovl))
	{
		auto gle = GetLastError();
		if (gle != ERROR_IO_PENDING)
		{
			LOG("Wiimote \"%s\": Failed to WriteFile: %d (0x%x)", m_Id.c_str(), gle, gle);
			return false;
		}
		if (!GetOverlappedResult(m_Handle, &ovl, &bw, TRUE))
		{
			auto gle = GetLastError();
			LOG("Wiimote \"%s\": Failed to GetOverlappedResult: %d (0x%x)", m_Id.c_str(), gle, gle);
			return false;
		}
	}
	if (bw != sizeof(buf))
	{
		LOG("Wiimote \"%s\": Failed to write buffer, %u bytes written out of %u", m_Id.c_str(), bw, static_cast<unsigned>(sizeof(buf)));
		return false;
	}
	return true;
}





unsigned char Wiimote::getRumbleBit() const
{
	return m_CurrentState.m_IsRumbleEnabled ? 0x01 : 0x00;
}




