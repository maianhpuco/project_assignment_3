#pragma once

#include "hittable.h"
#include "mat3.h"
#include "ray.h"
#include "vec3.h"

class material
{
public:
	virtual ~material() = default;

		// By default, surfaces do not emit light
		virtual color emitted(const hit_record&) const { return color(0, 0, 0); }

	struct scatter_record
	{
		color attenuation;
		ray	  scattered;
	};

		virtual bool scatter(const ray&, const hit_record&, scatter_record&) const { return false; }
		virtual color brdf_diffuse(const hit_record&) const { return color(0, 0, 0); }
};

class path_tracer_material : public material
{
public:
	using material::scatter_record;

	path_tracer_material(color albedo, float emission = 0.0f, color emission_color = color(1.0, 1.0, 1.0))
		  : _albedo(albedo), _emission(emission), _emission_color(emission_color)
	{}

	color emitted(const hit_record&) const override { return _emission > 0.0f ? _emission * _emission_color : color(0, 0, 0); }

	bool scatter(const ray& ray_in, const hit_record& record, scatter_record& srec) const override
	{
		(void)ray_in;
		if (_emission > 0.0f)
			return false;

		vec3 scatter_direction = record.normal + random_unit_vector();
		if (scatter_direction.near_zero())
			scatter_direction = record.normal;

		srec.scattered = ray(record.p, unit_vector(scatter_direction));
		srec.attenuation = _albedo;
		return true;
	}

	color brdf_diffuse(const hit_record&) const override
	{
		if (_emission > 0.0f)
			return color(0, 0, 0);
		return (_albedo / pi);
	}

private:
	color _albedo = color(1.0, 1.0, 1.0);
	float _emission = 0.0f;
	color _emission_color = color(1.0, 1.0, 1.0);
};
