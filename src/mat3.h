#pragma once

#include "vec3.h"

class mat3
{
public:
	mat3() = default;

	mat3(double e0, double e1, double e2, double e3, double e4, double e5, double e6, double e7, double e8)
	{
		e[0][0] = e0;
		e[0][1] = e1;
		e[0][2] = e2;
		e[1][0] = e3;
		e[1][1] = e4;
		e[1][2] = e5;
		e[2][0] = e6;
		e[2][1] = e7;
		e[2][2] = e8;
	}

	double& operator()(int i, int j) { return e[i][j]; }

	const double& operator()(int i, int j) const { return e[i][j]; }

	mat3 operator*(const mat3& m) const
	{
		return mat3(e[0][0] * m(0, 0) + e[0][1] * m(1, 0) + e[0][2] * m(2, 0),
					e[0][0] * m(0, 1) + e[0][1] * m(1, 1) + e[0][2] * m(2, 1),
					e[0][0] * m(0, 2) + e[0][1] * m(1, 2) + e[0][2] * m(2, 2),
					e[1][0] * m(0, 0) + e[1][1] * m(1, 0) + e[1][2] * m(2, 0),
					e[1][0] * m(0, 1) + e[1][1] * m(1, 1) + e[1][2] * m(2, 1),
					e[1][0] * m(0, 2) + e[1][1] * m(1, 2) + e[1][2] * m(2, 2),
					e[2][0] * m(0, 0) + e[2][1] * m(1, 0) + e[2][2] * m(2, 0),
					e[2][0] * m(0, 1) + e[2][1] * m(1, 1) + e[2][2] * m(2, 1),
					e[2][0] * m(0, 2) + e[2][1] * m(1, 2) + e[2][2] * m(2, 2));
	}

	vec3 operator*(const vec3& v) const
	{
		return vec3(e[0][0] * v.x() + e[0][1] * v.y() + e[0][2] * v.z(),
					e[1][0] * v.x() + e[1][1] * v.y() + e[1][2] * v.z(),
					e[2][0] * v.x() + e[2][1] * v.y() + e[2][2] * v.z());
	}

private:
	double e[3][3]{};
};