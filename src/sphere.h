#pragma once

#include "hittable.h"
#include "material.h"
#include "ray.h"

#include <memory>

class sphere : public hittable
{
public:
	sphere() = delete;

	explicit sphere(const point3& center, double radius, std::shared_ptr<material> mat);

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

	const point3& get_center() const { return center; }
	double		   get_radius() const { return radius; }
	std::shared_ptr<material> get_material() const { return mat; }

private:
	point3					  center;
	double					  radius;
	std::shared_ptr<material> mat;
};
