// Processor.cpp

// Declares the Processor class representing a single Wiimote's action processor that creates the mouse events.





#include "Globals.h"
#include "Processor.h"
#include "Warper.h"





Processor::Processor(const Warper & a_Warper, std::vector<WiimotePtr> & a_Wiimotes, const Wiimote * a_Wiimote):
	m_Warper(a_Warper)
{
	// Set up the callbacks:
	for (auto & w: a_Wiimotes)
	{
		if (w.get() == a_Wiimote)
		{
			m_Callback =
			[this](Wiimote & a_Wiimote)
			{
				auto irState = a_Wiimote.getCurrentIRState();
				if (irState.m_IsPresent1)
				{
					// The dot is visible, move the mouse:
					auto screenPt = m_Warper.warp(a_Wiimote, {irState.m_X1, irState.m_Y1});
					sendMouseInput(MOUSEEVENTF_MOVE, screenPt);
					if (!m_OldState.m_IsPresent1)
					{
						sendMouseInput(MOUSEEVENTF_LEFTDOWN, screenPt);
					}
				}
				else if (m_OldState.m_IsPresent1)
				{
					// The dot stopped being visible, emit a MouseUp
					auto screenPt = m_Warper.warp(a_Wiimote, {m_OldState.m_X1, m_OldState.m_Y1});
					sendMouseInput(MOUSEEVENTF_LEFTUP, screenPt);
				}
				m_OldState = irState;
			};
			w->addCallback(&m_Callback);
		}
	}
}





void Processor::sendMouseInput(DWORD a_Flags, POINT a_Pos)
{
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | a_Flags;
	input.mi.dx = a_Pos.x;
	input.mi.dy = a_Pos.y;
	input.mi.dwExtraInfo = 0;
	input.mi.mouseData = 0;
	input.mi.time = 0;
	SendInput(1, &input, sizeof(INPUT));
}




