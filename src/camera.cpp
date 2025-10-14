#include "camera.h"

#include "material.h"
#include "sphere.h"

#include <algorithm>
#include <cmath>

namespace
{
struct LightSample
{
	vec3  wi;
	double distance;
	double pdf;
	color emission;
};

bool sample_one_light(const point3& p, const vec3& normal, const hittable_list& world, LightSample& out)
{
	const auto& lights = world.lights();
	if (lights.empty())
		return false;

	int light_index = random_int(0, static_cast<int>(lights.size()) - 1);
	const auto& light = lights[light_index];

	point3 center = light->get_center();
	double radius = light->get_radius();
	vec3	 to_center = center - p;
	double distance_sq = to_center.length_squared();

	if (distance_sq <= radius * radius)
		return false;

	double cos_theta_max = std::sqrt(std::max(0.0, 1.0 - (radius * radius) / distance_sq));

	double r1 = random_double();
	double r2 = random_double();
	double phi = 2.0 * pi * r1;
	double cos_theta = 1.0 - r2 + r2 * cos_theta_max;
	double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));

	vec3 local_dir(std::cos(phi) * sin_theta,
				   std::sin(phi) * sin_theta,
				   cos_theta);

	vec3 w = unit_vector(to_center);
	vec3 helper = (std::fabs(w.x()) > 0.1) ? vec3(0, 1, 0) : vec3(1, 0, 0);
	vec3 u = unit_vector(cross(helper, w));
	vec3 v = cross(w, u);

	vec3 dir = unit_vector(u * local_dir.x() + v * local_dir.y() + w * local_dir.z());
	ray  shadow_ray(p, dir);

	hit_record light_hit;
	if (!light->hit(shadow_ray, interval(0.001, infinity), light_hit))
		return false;

	if (dot(normal, dir) <= 0.0)
		return false;

	out.wi = dir;
	out.distance = light_hit.t;
	out.pdf = 1.0 / (2.0 * pi * (1.0 - cos_theta_max));
	out.emission = light_hit.mat->emitted(light_hit);

	return out.pdf > 0.0;
}
} // namespace

camera::~camera()
{
}

camera::camera()
{
}

void camera::init()
{
	image_height = int(image_width / aspect_ratio);
	image_height = (image_height < 1) ? 1 : image_height;

	center = lookfrom;

	// Determine viewport dimensions.
	auto focal_length = (lookfrom - lookat).length();
	auto theta = degrees_to_radians(vfov);
	auto h = std::tan(theta / 2);
	auto viewport_height = 2 * h * focal_length;
	auto viewport_width = viewport_height * (double(image_width) / image_height);

	// Calculate the u,v,w unit basis vectors for the camera coordinate frame.
	w = unit_vector(lookfrom - lookat);
	u = unit_vector(cross(vup, w));
	v = cross(w, u);

	// Calculate the vectors across the horizontal and down the vertical viewport edges.
	vec3 viewport_u = viewport_width * u;	 // Vector across viewport horizontal edge
	vec3 viewport_v = viewport_height * -v;	 // Vector down viewport vertical edge

	// Calculate the horizontal and vertical delta vectors from pixel to pixel.
	pixel_delta_u = viewport_u / image_width;
	pixel_delta_v = viewport_v / image_height;

	// Calculate the location of the upper left pixel.
	auto viewport_upper_left = center - (focal_length * w) - viewport_u / 2 - viewport_v / 2;
	pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
}

ray camera::generate_ray(int xi, int yi) const
{
	auto offset = sample_square();
	auto pixel_sample = pixel00_loc
			+ ((xi + offset.x()) * pixel_delta_u)
			+ ((yi + offset.y()) * pixel_delta_v);

	auto ray_origin = center;
	auto ray_direction = pixel_sample - ray_origin;

	return ray(ray_origin, ray_direction);
}

color camera::ray_color(const ray& r, const hittable& world, const int depth)
{
    if (depth <= 0)
        return color(0, 0, 0);

    hit_record record;
    if (!world.hit(r, interval(0.001, infinity), record))
    {
        // Black background so emission and shadows are visible
        return color(0, 0, 0);
    }

	// Emission from light materials
	color emit = record.mat->emitted(record);

	// Direct lighting with next-event estimation
	color direct_light = color(0, 0, 0);
	if (const auto* world_list = dynamic_cast<const hittable_list*>(&world))
	{
		LightSample sample;
		if (sample_one_light(record.p, record.normal, *world_list, sample))
		{
			hit_record occluder;
			double	   max_distance = std::max(0.0, sample.distance - 1e-4);
			ray		   shadow(record.p, sample.wi);
			if (max_distance > 0.001 && !world.hit(shadow, interval(0.001, max_distance), occluder))
			{
				double cos_out = std::max(0.0, dot(record.normal, sample.wi));
				if (cos_out > 0.0)
				{
					color brdf = record.mat->brdf_diffuse(record);
					direct_light = sample.emission * brdf * (cos_out / std::max(sample.pdf, 1e-8));
				}
			}
		}
	}

	material::scatter_record srec;
	if (!record.mat->scatter(r, record, srec))
	{
		return emit + direct_light;
	}

	return emit + direct_light + srec.attenuation * ray_color(srec.scattered, world, depth - 1);
}

vec3 camera::sample_square() const
{
	// Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
	return vec3(random_double() - 0.5, random_double() - 0.5, 0);
}
