// Processor.h

// Declares the Processor class representing a single Wiimote's action processor that creates the mouse events.





#pragma once





#include "Wiimote.h"





// fwd:
class Warper;





class Processor
{
public:
	Processor(const Warper & a_Warper, std::vector<WiimotePtr> & a_Wiimotes, const Wiimote * a_Wiimote);

protected:
	const Warper & m_Warper;

	Wiimote::IRState m_OldState;

	Wiimote::Callback m_Callback;

	/** Sends the mouse input event with the specified flags and position.
	Always adds the MOUSEEVENTF_ABSOLUTE flag. */
	void sendMouseInput(DWORD a_Flags, POINT a_Pos);
};

typedef std::shared_ptr<Processor> ProcessorPtr;



