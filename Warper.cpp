// Warper.cpp

// Implements the Warper class representing the engine that turns Wiimote coords into screen coords, based on calibration





#include "Globals.h"
#include "Warper.h"
#include "Calibration.h"





static const Warper::Matrix::Number EPS = static_cast<Warper::Matrix::Number>(0.00001);





////////////////////////////////////////////////////////////////////////////////
// Warper::Matrix:

bool Warper::Matrix::squareToQuad(
	Number a_DstX1, Number a_DstY1,
	Number a_DstX2, Number a_DstY2,
	Number a_DstX3, Number a_DstY3,
	Number a_DstX4, Number a_DstY4
)
{
	Number ax = a_DstX1 - a_DstX2 + a_DstX3 - a_DstX4;
	Number ay = a_DstY1 - a_DstY2 + a_DstY3 - a_DstY4;

	if ((std::abs(ax) < EPS) && (std::abs(ay) < EPS))
	{
		// afine transform
		m_Matrix[0][0] = a_DstX2 - a_DstX1;
		m_Matrix[0][1] = a_DstY2 - a_DstY1;
		m_Matrix[0][2] = 0;
		m_Matrix[1][0] = a_DstX3 - a_DstX2;
		m_Matrix[1][1] = a_DstY3 - a_DstY2;
		m_Matrix[1][2] = 0;
		m_Matrix[2][0] = a_DstX1;
		m_Matrix[2][1] = a_DstY1;
		m_Matrix[2][2] = 1;
	}
	else
	{
		Number ax1 = a_DstX2 - a_DstX3;
		Number ax2 = a_DstX4 - a_DstX3;
		Number ay1 = a_DstY2 - a_DstY3;
		Number ay2 = a_DstY4 - a_DstY3;

		// Sub-determinants
		Number gtop = ax  * ay2 - ax2 * ay;
		Number htop = ax1 * ay - ax  * ay1;
		Number bottom = ax1 * ay2 - ax2 * ay1;

		if (std::abs(bottom) < EPS)
		{
			return false;
		}

		Number g = gtop / bottom;
		Number h = htop / bottom;

		m_Matrix[0][0] = a_DstX2 - a_DstX1 + g * a_DstX2;
		m_Matrix[1][0] = a_DstX4 - a_DstX1 + h * a_DstX4;
		m_Matrix[2][0] = a_DstX1;
		m_Matrix[0][1] = a_DstY2 - a_DstY1 + g * a_DstY2;
		m_Matrix[1][1] = a_DstY4 - a_DstY1 + h * a_DstY4;
		m_Matrix[2][1] = a_DstY1;

		m_Matrix[0][2] = g;
		m_Matrix[1][2] = h;
		m_Matrix[2][2] = 1;
	}
	return true;
}





bool Warper::Matrix::quadToSquare(
	Number a_SrcX1, Number a_SrcY1,
	Number a_SrcX2, Number a_SrcY2,
	Number a_SrcX3, Number a_SrcY3,
	Number a_SrcX4, Number a_SrcY4
)
{
	// Compute the Square-to-Quad transform and invert it:
	if (!squareToQuad(a_SrcX1, a_SrcY1, a_SrcX2, a_SrcY2, a_SrcX3, a_SrcY3, a_SrcX4, a_SrcY4))
	{
		return false;
	}
	if (!invert())
	{
		return false;
	}
	return true;
}





bool Warper::Matrix::invert()
{
	Number det = determinant();
	if (std::abs(det) < EPS)
	{
		return false;
	}
	Number h11 = m_Matrix[1][1] * m_Matrix[2][2] - m_Matrix[1][2] * m_Matrix[2][1];
	Number h21 = m_Matrix[1][2] * m_Matrix[2][0] - m_Matrix[1][0] * m_Matrix[2][2];
	Number h31 = m_Matrix[1][0] * m_Matrix[2][1] - m_Matrix[1][1] * m_Matrix[2][0];
	Number h12 = m_Matrix[0][2] * m_Matrix[2][1] - m_Matrix[0][1] * m_Matrix[2][2];
	Number h22 = m_Matrix[0][0] * m_Matrix[2][2] - m_Matrix[0][2] * m_Matrix[2][0];
	Number h32 = m_Matrix[0][1] * m_Matrix[2][0] - m_Matrix[0][0] * m_Matrix[2][1];
	Number h13 = m_Matrix[0][1] * m_Matrix[1][2] - m_Matrix[0][2] * m_Matrix[1][1];
	Number h23 = m_Matrix[0][2] * m_Matrix[1][0] - m_Matrix[0][0] * m_Matrix[1][2];
	Number h33 = m_Matrix[0][0] * m_Matrix[1][1] - m_Matrix[0][1] * m_Matrix[1][0];

	m_Matrix[0][0] = h11 / det;
	m_Matrix[1][0] = h21 / det;
	m_Matrix[2][0] = h31 / det;
	m_Matrix[0][1] = h12 / det;
	m_Matrix[1][1] = h22 / det;
	m_Matrix[2][1] = h32 / det;
	m_Matrix[0][2] = h13 / det;
	m_Matrix[1][2] = h23 / det;
	m_Matrix[2][2] = h33 / det;
	return true;
}





Warper::Matrix::Number Warper::Matrix::determinant() const
{
  return (
		m_Matrix[0][0] * (m_Matrix[2][2] * m_Matrix[1][1] - m_Matrix[2][1] * m_Matrix[1][2]) -
    m_Matrix[1][0] * (m_Matrix[2][2] * m_Matrix[0][1] - m_Matrix[2][1] * m_Matrix[0][2]) +
		m_Matrix[2][0] * (m_Matrix[1][2] * m_Matrix[0][1] - m_Matrix[1][1] * m_Matrix[0][2])
	);
}





void Warper::Matrix::multiplyBy(const Warper::Matrix & a_Other)
{
	Number m11 = m_Matrix[0][0] * a_Other.m_Matrix[0][0] + m_Matrix[0][1] * a_Other.m_Matrix[1][0] + m_Matrix[0][2] * a_Other.m_Matrix[2][0];
	Number m12 = m_Matrix[0][0] * a_Other.m_Matrix[0][1] + m_Matrix[0][1] * a_Other.m_Matrix[1][1] + m_Matrix[0][2] * a_Other.m_Matrix[2][1];
	Number m13 = m_Matrix[0][0] * a_Other.m_Matrix[0][2] + m_Matrix[0][1] * a_Other.m_Matrix[1][2] + m_Matrix[0][2] * a_Other.m_Matrix[2][2];
	Number m21 = m_Matrix[1][0] * a_Other.m_Matrix[0][0] + m_Matrix[1][1] * a_Other.m_Matrix[1][0] + m_Matrix[1][2] * a_Other.m_Matrix[2][0];
	Number m22 = m_Matrix[1][0] * a_Other.m_Matrix[0][1] + m_Matrix[1][1] * a_Other.m_Matrix[1][1] + m_Matrix[1][2] * a_Other.m_Matrix[2][1];
	Number m23 = m_Matrix[1][0] * a_Other.m_Matrix[0][2] + m_Matrix[1][1] * a_Other.m_Matrix[1][2] + m_Matrix[1][2] * a_Other.m_Matrix[2][2];
	Number m31 = m_Matrix[2][0] * a_Other.m_Matrix[0][0] + m_Matrix[2][1] * a_Other.m_Matrix[1][0] + m_Matrix[2][2] * a_Other.m_Matrix[2][0];
	Number m32 = m_Matrix[2][0] * a_Other.m_Matrix[0][1] + m_Matrix[2][1] * a_Other.m_Matrix[1][1] + m_Matrix[2][2] * a_Other.m_Matrix[2][1];
	Number m33 = m_Matrix[2][0] * a_Other.m_Matrix[0][2] + m_Matrix[2][1] * a_Other.m_Matrix[1][2] + m_Matrix[2][2] * a_Other.m_Matrix[2][2];

	m_Matrix[0][0] = m11;
	m_Matrix[1][0] = m21;
	m_Matrix[2][0] = m31;
	m_Matrix[0][1] = m12;
	m_Matrix[1][1] = m22;
	m_Matrix[2][1] = m32;
	m_Matrix[0][2] = m13;
	m_Matrix[1][2] = m23;
	m_Matrix[2][2] = m33;
}





POINT Warper::Matrix::project(POINT a_Src) const
{
	auto res = project(static_cast<Number>(a_Src.x), static_cast<Number>(a_Src.y));
	return
	{
		static_cast<LONG>(res.first),
		static_cast<LONG>(res.second)
	};
}





std::pair<Warper::Matrix::Number, Warper::Matrix::Number> Warper::Matrix::project(Number a_X, Number a_Y) const
{
	Number nx = m_Matrix[0][0] * a_X + m_Matrix[1][0] * a_Y + m_Matrix[2][0];
	Number ny = m_Matrix[0][1] * a_X + m_Matrix[1][1] * a_Y + m_Matrix[2][1];
	Number w  = m_Matrix[0][2] * a_X + m_Matrix[1][2] * a_Y + m_Matrix[2][2];
	if (w < EPS)
	{
		w = EPS;
	}
	return std::make_pair(nx / w, ny / w);
}





////////////////////////////////////////////////////////////////////////////////
// Warper:

Warper::Warper()
{
	// Nothing needed yet
}





void Warper::setCalibration(const Calibration & a_Calibration)
{
	m_Matrices.clear();
	for (const auto & mapping: a_Calibration.getMappings())
	{
		if (!mapping.second.isUsable())
		{
			continue;
		}
		auto & matrix = m_Matrices[mapping.first];

		// Project from Wiimote coords to screen coords via a unit square:
		const auto & points = mapping.second.m_Points;
		matrix.quadToSquare(
			static_cast<Matrix::Number>(points[0].m_WiimoteX), static_cast<Matrix::Number>(points[0].m_WiimoteY),
			static_cast<Matrix::Number>(points[1].m_WiimoteX), static_cast<Matrix::Number>(points[1].m_WiimoteY),
			static_cast<Matrix::Number>(points[2].m_WiimoteX), static_cast<Matrix::Number>(points[2].m_WiimoteY),
			static_cast<Matrix::Number>(points[3].m_WiimoteX), static_cast<Matrix::Number>(points[3].m_WiimoteY)
		);
		Matrix helper;
		helper.squareToQuad(
			static_cast<Matrix::Number>(points[0].m_ScreenX), static_cast<Matrix::Number>(points[0].m_ScreenY),
			static_cast<Matrix::Number>(points[1].m_ScreenX), static_cast<Matrix::Number>(points[1].m_ScreenY),
			static_cast<Matrix::Number>(points[2].m_ScreenX), static_cast<Matrix::Number>(points[2].m_ScreenY),
			static_cast<Matrix::Number>(points[3].m_ScreenX), static_cast<Matrix::Number>(points[3].m_ScreenY)
		);
		matrix.multiplyBy(helper);
	}  // for mapping - a_Calibration[]
}




std::vector<const Wiimote *> Warper::getWarpableWiimotes() const
{
	std::vector<const Wiimote *> res;
	for (const auto & w: m_Matrices)
	{
		res.push_back(w.first);
	}
	return res;
}





POINT Warper::warp(Wiimote & a_Wiimote, POINT a_WiimotePoint) const
{
	
	const auto itr = m_Matrices.find(&a_Wiimote);
	assert(itr != m_Matrices.end());
	auto res = itr->second.project(static_cast<Matrix::Number>(a_WiimotePoint.x), static_cast<Matrix::Number>(a_WiimotePoint.y));
	return
	{
		static_cast<LONG>(std::floor(res.first  + static_cast<Matrix::Number>(0.5))),
		static_cast<LONG>(std::floor(res.second + static_cast<Matrix::Number>(0.5)))
	};
}




