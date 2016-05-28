// Wiimote.h

// Declares the Wiimote class representing a single connected Wiimote controller





#pragma once





#include <thread>
#include <mutex>
#include <condition_variable>





class Wiimote
{
public:
	/** The DevicePath, represented as Utf8. Use WiimoteManager to enum Ids.*/
	typedef std::string Id;

	/** Container for multiple Ids. */
	typedef std::vector<Wiimote::Id> Ids;


	/** Interface that the clients of this class use to get notified of changes in the Wiimote state.
	The parameter is the wiimote for which the callback is being called. */
	typedef std::function<void (Wiimote &)> Callback;


	/** The input report type, received from the Wiimote. */
	enum InputReportType
	{
		irtButtons          = 0x30,
		irtButtonsAccel     = 0x31,
		irtButtonsExtension = 0x34,
		irtExtensionAccel   = 0x35,
		irtIRAccel          = 0x33,
		irtIRExtensionAccel = 0x37,
		irtReadData         = 0x21,
		irtStatus           = 0x20,
	};


	enum IRReportingMode
	{
		irrmOff      = 0x00,
		irrmBasic    = 0x01,
		irrmExtended = 0x03,
		irrmFull     = 0x05,
	};


	/** Representation of the state of all the buttons. */
	struct ButtonState
	{
		// true = button pressed
		bool m_ButtonA, m_ButtonB;
		bool m_ButtonHome;
		bool m_ButtonMinus, m_ButtonPlus;
		bool m_ButtonOne, m_ButtonTwo;
		bool m_ButtonUp, m_ButtonDown, m_ButtonLeft, m_ButtonRight;
	};


	/** Representation of the IR camera state */
	struct IRState
	{
		// The dots' coordinates:
		int m_X1, m_Y1;
		int m_X2, m_Y2;
		int m_X3, m_Y3;
		int m_X4, m_Y4;

		// Flags for the dots' presence:
		bool m_IsPresent1, m_IsPresent2, m_IsPresent3, m_IsPresent4;

		// Reporting mode of the IR camera (Basic / Extended / Full):
		IRReportingMode m_ReportingMode;
	};


	/** Representation of the state of the accelerometers. */
	struct AccelState
	{
		// Acceleration around the axes, 0x80 ~ zero
		unsigned short m_AccelX, m_AccelY, m_AccelZ;
	};


	/** Representation of the accelerometer calibration data. */
	struct AccelCalibration
	{
		// Zero point:
		unsigned char m_X0, m_Y0, m_Z0;

		// Gravity at rest:
		unsigned char m_XG, m_YG, m_ZG;
	};


	/** Representation of the entire state of the Wiimote. */
	struct State
	{
		ButtonState m_ButtonState;
		IRState m_IRState;
		AccelState m_AccelState;
		AccelCalibration m_AccelCalibration;
		bool m_IsRumbleEnabled;
		bool m_Led1, m_Led2, m_Led3, m_Led4;
	};


	// Identification of the Wiimote
	static const USHORT VendorID = 0x057e;
	static const USHORT ProductID = 0x0306;


	/** Creates a new empty Wiimote instance. */
	Wiimote();
	
	~Wiimote();

	/** Connects to the specified Wiimote Id.
	Returns true if the connection succeeded.
	a_InitialCallback may be filled to provide the callback from the very beginning of the object's lifetime. */
	bool connect(const Id & a_Id, Callback * a_InitialCallback);

	/** Converts the UTF-16 device path used by the OS into a Wiimote Id. */
	static Id IdFromWPath(LPCWSTR a_DevicePath);

	/** Converts the Wiimote Id into a UTF-16 device path used by the OS. */
	static std::wstring WPathFromId(const Id & a_Id);

	/** Returns the current state of the controller. */
	State getCurrentState() const;

	/** Returns the current IR camera state. */
	IRState getCurrentIRState() const;

	/** Sets the LEDs to the specified states. */
	void setLeds(bool a_Led1, bool a_Led2, bool a_Led3, bool a_Led4);

	/** Sets the reporting mode to the specified type.
	Asks the Wiimote to send reports of the specified type. */
	void setReportType(InputReportType a_Type, bool a_Continuous);

	/** Adds the specified callback to the container of callbacks called for detected changes. */
	void addCallback(Callback * a_Callback);

	/** Removes the specified callback from the container of callbacks called for detected changes. */
	void removeCallback(Callback * a_Callback);


protected:

	/** Type used for storing read data requests. The DataRead packets read from the Wiimote will get stored in "first" and then "second" will be notified. */
	typedef std::tuple<char *, short, std::condition_variable *> ReadDataRequest;

	/** The Id of the controller. */
	Id m_Id;

	/** OS handle for the Wiimote device. */
	HANDLE m_Handle;

	/** Flag indicating that the read thread should terminate as soon as possible. */
	bool m_ShouldTerminate;

	/** Mutex protecting the m_CurrentState and m_ReadDataRequests against multithreaded access. */
	mutable std::mutex m_CS;

	/** Current state of the controller.
	Protected against multithreaded access by m_CS. */
	State m_CurrentState;

	/** The thread used for reading from the Wiimote. */
	std::thread m_ReadThread;

	/** Callbacks that should be called whenever the state changes. */
	std::list<Callback *> m_Callbacks;

	/** Indicates whether writes to the Wiimote should use an alternate method.
	Auto-detected at start time. */
	bool m_UseAltWrite;

	/** FIFO queue of requests for reading data from the Wiimote.
	Whenever an irtReadData report comes in, the front request is filled with the data and its condvar notified if all data was read.
	Protected against multithreaded access with m_CS*/
	std::list<ReadDataRequest> m_ReadDataRequests;

	/** Mutex serializing write operations. */
	std::mutex m_CSWrite;

	/** The event used for IO completion in write operations. */
	HANDLE m_WriteEvent;

	/** The time when the last write operation was started.
	Access serialized by m_CSWrite. */
	std::chrono::system_clock::time_point m_LastWriteTime;


	/** Enables the IR reporting in the specified format on the Wiimote. */
	void enableIR(IRReportingMode a_Mode);

	/** Disables the IR reporting on the Wiimote. */
	void disableIR();

	/** Parses the packet received from the Wiimote and updates m_CurrentState accordingly.
	Returns true if the packet was parsed successfully (state has been updated). */
	bool parseIncomingPacket(const unsigned char * a_Packet);

	/** Parses the accelerometer data from the packet. */
	void parseAccel(const unsigned char * a_Packet);

	/** Parses the button data from the packet. */
	void parseButtons(const unsigned char * a_Packet);

	/** Parses the IR data from the packet. */
	void parseIR(const unsigned char * a_Packet);

	/** Parses the irtReadData input report packet. */
	void parseReadData(const unsigned char * a_Packet);

	/** Reads data incoming from the Wiimote.
	Executed in m_ReadThread. */
	void thrRead();

	/** Notifies all callbacks that the internal state has been changed.
	Runs in m_ReadThread. */
	void notifyStateChange();

	/** Requests and reads the calibration from the Wiimote, effectively initializing it.
	Returns true if the read succeeded, false on failure. */
	bool readCalibration();

	/** Reads data from the specified address into the specified buffer.
	Returns true if the read succeeded, false otherwise.
	Blocks until the read completes or times out. */
	bool readData(int a_Address, short a_Size, char * a_Buffer);

	/** Writes a single data byte to the specified address. */
	bool writeData(int a_Address, unsigned char a_Value);

	/** Writes a sequence of bytes to the specified address. */
	bool writeData(int a_Address, const void * a_Values, unsigned char a_Size);

	/** Writes the specified output report to the Wiimote. */
	bool writeReport(const void * a_Buffer, size_t a_Size);

	/** Returns the current state of the Rumble bit encoded in the output report format. */
	unsigned char getRumbleBit() const;
};

typedef std::shared_ptr<Wiimote> WiimotePtr;
typedef std::vector<WiimotePtr> WiimotePtrs;




