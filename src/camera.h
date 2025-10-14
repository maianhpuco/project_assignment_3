#pragma once

#include "hittable.h"
#include "hittable_list.h"
#include "ray.h"
#include "utility.h"
#include "vec3.h"

class camera
{
public:
	~camera();

	camera();

	double aspect_ratio = 1.0;
	int	   image_width = 100;
	double vfov = 90;					// Vertical view angle (field of view)
	point3 lookfrom = point3(0, 0, 0);	// Point camera is looking from
	point3 lookat = point3(0, 0, -1);	// Point camera is looking at
	vec3   vup = vec3(0, 1, 0);			// Camera-relative "up" direction
	vec3   u, v, w;						// Camera frame basis vectors

	void init();

	int get_image_height() const { return image_height; }

	int get_image_width() const { return image_width; }

	ray generate_ray(int xi, int yi) const;

	// Expose ray_color so the renderer can reuse the camera's integrator
	color ray_color(const ray& r, const hittable& world, const int depth);

private:
	int	   image_height;
	point3 center;
	point3 pixel00_loc;
	vec3   pixel_delta_u;
	vec3   pixel_delta_v;


	vec3 sample_square() const;
};