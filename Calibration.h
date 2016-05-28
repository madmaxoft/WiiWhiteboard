// Calibration.h

// Declares the Calibration class representing the wiimote-to-screen calibration data





#pragma once





#include "Wiimote.h"





class Calibration
{
public:
	/** Contains a single point of correspondence. */
	struct CorrespondingPoint
	{
		bool m_IsValid;
		int m_ScreenX;
		int m_ScreenY;
		int m_WiimoteX;
		int m_WiimoteY;

		CorrespondingPoint():
			m_IsValid(false)
		{
		}
	};

	/** Contains correspondence for a single Wiimote */
	struct Mapping
	{
		const Wiimote * m_Wiimote;
		CorrespondingPoint m_Points[4];

		/** Returns true if the mapping is complete - all 4 points are defined. */
		bool isUsable() const;
	};

	typedef std::map<const Wiimote *, Mapping> Mappings;


	Calibration();

	/** Sets a correspondence point between a screen calibration point, and a wiimote IR coord. */
	void setPoint(
		Wiimote & a_Wiimote,
		int a_CalibrationPointIndex,
		int a_WiimoteX, int a_WiimoteY,
		int a_ScreenX, int a_ScreenY
	);

	/** Returns true if there is at least one complete mapping*/
	bool isUsable() const;

	const Mappings & getMappings() const { return m_Mappings; }


protected:

	/** All known mappings. */
	Mappings m_Mappings;


	/** Returns a (writable) mapping for the specified Wiimote.
	If there wasn't a mapping, creates a new one. */
	Mapping & getWiimoteMapping(const Wiimote & a_Wiimote);
};

typedef std::shared_ptr<Calibration> CalibrationPtr;




