// Calibration.cpp

// Implements the Calibration class representing the wiimote-to-screen calibration data





#include "Globals.h"
#include "Calibration.h"





////////////////////////////////////////////////////////////////////////////////
// Calibration::Mapping:





bool Calibration::Mapping::isUsable() const
{
	return (m_Points[0].m_IsValid && m_Points[1].m_IsValid && m_Points[2].m_IsValid && m_Points[3].m_IsValid);
}





////////////////////////////////////////////////////////////////////////////////
// Calibration:

Calibration::Calibration()
{
}





void Calibration::setPoint(
	Wiimote & a_Wiimote,
	int a_CalibrationPointIndex,
	int a_WiimoteX, int a_WiimoteY,
	int a_ScreenX, int a_ScreenY
)
{
	auto & mapping = getWiimoteMapping(a_Wiimote);
	auto & point = mapping.m_Points[a_CalibrationPointIndex];
	point.m_IsValid = true;
	point.m_WiimoteX = a_WiimoteX;
	point.m_WiimoteY = a_WiimoteY;
	point.m_ScreenX = a_ScreenX;
	point.m_ScreenY = a_ScreenY;
}





bool Calibration::isUsable() const
{
	for (const auto & mapping: m_Mappings)
	{
		if (mapping.second.isUsable())
		{
			return true;
		}
	}
	return false;
}





Calibration::Mapping & Calibration::getWiimoteMapping(const Wiimote & a_Wiimote)
{
	auto itr = m_Mappings.find(&a_Wiimote);
	if (itr == m_Mappings.end())
	{
		// No such mapping yet, create a new one:
		Mapping mapping;
		mapping.m_Wiimote = &a_Wiimote;
		itr = m_Mappings.insert(m_Mappings.begin(), std::make_pair(&a_Wiimote, mapping));
	}
	return itr->second;
}
