#pragma once

#include "hittable.h"
#include "interval.h"

#include <memory>
#include <vector>

class sphere;

class hittable_list : public hittable
{
public:
	std::vector<std::shared_ptr<hittable>> objects;
	hittable_list() = default;
	explicit hittable_list(std::shared_ptr<hittable> object);

	void clear();
	void add(std::shared_ptr<hittable> object);
	void add_light(const std::shared_ptr<sphere>& light);
	const std::vector<std::shared_ptr<sphere>>& lights() const { return light_sources; }

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

private:
	std::vector<std::shared_ptr<sphere>> light_sources;
};
