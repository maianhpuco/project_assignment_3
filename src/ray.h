#pragma once

#include "mat3.h"
#include "vec3.h"

class ray
{
public:
	ray() {}

	ray(const vec3& origin, const vec3& direction)
		  : m_orig(origin), m_dir(direction)
	{
	}

	vec3 origin() const { return m_orig; }

	vec3 direction() const { return m_dir; }

	vec3 at(float t) const { return m_orig + t * m_dir; }

private:
	vec3 m_orig;
	vec3 m_dir;
};

inline ray sample_hemisphere(const point3& p, const vec3& normal)
{
	auto x0 = random_double(0.0, 1.0);
	auto x1 = random_double(0.0, 1.0);
	auto phi = 2 * pi * x1;
	auto theta = std::acos(std::sqrt(1 - x0));
	auto sin_theta = std::sin(theta);
	auto cos_theta = std::cos(theta);
	auto sin_phi = std::sin(phi);
	auto cos_phi = std::cos(phi);

	vec3 v(sin_theta * cos_phi,
		   sin_theta * sin_phi,
		   cos_theta);

	auto a = cross(normal, vec3(0, 0, 1));
	auto s = a / a.length();
	auto cospeta = dot(s, normal);
	auto sinpeta = std::sqrt(1 - cospeta * cospeta);

	auto sx = s.x();
	auto sy = s.y();
	auto sz = s.z();

	mat3 R(sx * sx * (1 - cospeta) + cospeta,
		   sx * sy * (1 - cospeta) - sz * sinpeta,
		   sx * sz * (1 - cospeta) + sy * sinpeta,
		   sy * sx * (1 - cospeta) + sz * sinpeta,
		   sy * sy * (1 - cospeta) + cospeta,
		   sy * sz * (1 - cospeta) - sx * sinpeta,
		   sz * sx * (1 - cospeta) - sy * sinpeta,
		   sz * sy * (1 - cospeta) + sx * sinpeta,
		   sz * sz * (1 - cospeta) + cospeta);

	return ray(p, R * v);
}