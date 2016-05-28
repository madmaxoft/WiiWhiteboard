// Warper.h

// Declares the Warper class representing the engine that turns Wiimote coords into screen coords, based on calibration





#pragma once





#include "Calibration.h"





// fwd:
class Wiimote;





class Warper
{
public:
	Warper();

	/** Calculates the projection matrices for each usable Wiimote in the specified Calibration. */
	void setCalibration(const Calibration & a_Calibration);

	/** Returns a vector of all Wiimotes that have a valid warping established. */
	std::vector<const Wiimote *> getWarpableWiimotes() const;

	/** Warps the specified point using the specified Wiimote's warping.
	Assumes the Wiimote has a valid warping (asserts). */
	POINT warp(Wiimote & a_Wiimote, POINT a_WiimotePoint) const;

// TODO
// protected:

	class Matrix
	{
	public:
		typedef float Number;

		/** Sets this matrix to be a projection from a unit square to a quad of the specified coords.
		Returns true if the transform is possible, false if not. */
		bool squareToQuad(
			Number a_DstX1, Number a_DstY1,
			Number a_DstX2, Number a_DstY2,
			Number a_DstX3, Number a_DstY3,
			Number a_DstX4, Number a_DstY4
		);

		/** Sets this matrix to be a projection from a quad of the specified coords to a unit square.
		Returns true if the transform is possible, false if not. */
		bool quadToSquare(
			Number a_SrcX1, Number a_SrcY1,
			Number a_SrcX2, Number a_SrcY2,
			Number a_SrcX3, Number a_SrcY3,
			Number a_SrcX4, Number a_SrcY4
		);

		/** Inverts this matrix.
		Returns true if inversion was possible, false if not. */
		bool invert();

		/** Returns the determinant of the matrix. */
		Number determinant() const;

		/** Multiplies this matrix by another one. */
		void multiplyBy(const Matrix & a_Other);

		/** Returns the coords of the specified point projected by this matrix. */
		POINT project(POINT a_Src) const;

		std::pair<Number, Number> project(Number a_X, Number a_Y) const;

	protected:
		Number m_Matrix[3][3];
	};


	class Params
	{
	public:
		typedef float Number;


		void set(const Calibration::Mapping & a_Calibration);

		std::pair<Number, Number> project(Number a_X, Number a_Y) const;

	protected:
		Number m_WiimoteXA, m_WiimoteXAB, m_WiimoteXBC, m_WiimoteXAD;
		Number m_WiimoteYA, m_WiimoteYAB, m_WiimoteYBC, m_WiimoteYAD;
		Number m_ScreenXA, m_ScreenXAB, m_ScreenXBC, m_ScreenXAD;
		Number m_ScreenYA, m_ScreenYAB, m_ScreenYBC, m_ScreenYAD;
	};

	/** Maps a specific Wiimote to its projection matrix. */
	typedef std::map<const Wiimote *, Matrix> MatrixMap;

	typedef std::map<const Wiimote *, Params> ParamsMap;


	/** Individual Wiimotes' projection matrices. */
	MatrixMap m_Matrices;

	ParamsMap m_Params;
};

