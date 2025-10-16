#pragma once

#include "../SDK/Misc/QAngle.h"
#include "../SDK/Misc/Vector.h"
#include "../SDK/Misc/Matrix.h"

#include <algorithm>

#define M_PI 3.14159265358979323846
#define PI_F	((float)(M_PI)) 
#define DEG2RAD(deg) ((deg) * (PI_F / 180.f))
#define RAD2DEG(rad) ((rad) / (PI_F / 180.f))
#define fsel(c,x,y) ( (c) >= 0 ? (x) : (y) )

namespace Math {
	inline float Q_sqrt(const float number) {
		const unsigned int i = (*(unsigned int*)&number + 0x3f800000) >> 1;
		const float approx = *(float*)&i;
		return (approx + number / approx) * 0.5f;
	}

	inline float RemapVal(float val, float A, float B, float C, float D)
	{
		if (A == B)
			return (val - B) >= 0 ? D : C;
		return C + (D - C) * (val - A) / (B - A);
	}

	inline float RemapValClamped(float val, float A, float B, float C, float D) {
		if (A == B)
			return fsel(val - B, D, C);
		float cVal = (val - A) / (B - A);
		cVal = std::clamp<float>(cVal, 0.0f, 1.0f);

		return C + (D - C) * cVal;
	}

	void RotateTrianglePoints(Vector2 points[3], float rotation);

	Vector AngleFromVectors(Vector a, Vector b);

	float			Lerp(float a, float b, float perc);
	float			AngleNormalize(float angle);
	float           NormalizeYaw(float f);
	float			AngleNormalizePositive(float angle);
	float			AngleDiff(float next, float cur);
	float			AngleToPositive(float angle);
	void			AngleVectors(const QAngle& angles, Vector& forward, Vector& right, Vector& up);
	void			AngleVectors(const QAngle& angles, Vector& forward);
	Vector			AngleVectors(const QAngle& angles);
	QAngle			VectorAngles(const Vector& vec);
	QAngle			VectorAngles_p(const Vector& vec);
	void			VectorTransform(const Vector& in, const matrix3x4_t& matrix, Vector* out);
	Vector			VectorTransform(const Vector& in, const matrix3x4_t& matrix);
	Vector			VectorRotate(const Vector& in, const matrix3x4_t& matrix);
	Vector			VectorRotate(const Vector& in, const QAngle& rotate);
}