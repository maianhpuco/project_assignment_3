#pragma once

#include "camera.h"
#include "hittable_list.h"
#include "threadpool.h"

#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>

class renderer final
{
public:
	using render_callback =
			std::function<void(std::size_t, color&, float transparent)>;

	renderer() = delete;

	// Rule of 5, for class containing unique pointers
	renderer(const renderer& other);
	renderer& operator=(const renderer& other);
renderer(renderer&& other) noexcept = delete;
renderer& operator=(renderer&& other) noexcept = delete;

	~renderer();

	explicit renderer(std::shared_ptr<camera> cam, std::shared_ptr<hittable_list> world, const render_callback& callback);

	void init(int numofthreads, int	   _numOfBounces,
	int _pixels_height_per_thread,
	int _pixels_width_per_thread,
	int	   _numOfThreads,
	bool	_is_accumulation);

	void start_rendering();
	void stop_rendering();

	void resize_sample();
	void change_num_of_bounces(int n);
	void change_render_ratio(int w, int h);
	void change_num_of_threads(int n);
	void set_accumulation(bool on);
	bool get_accumulation() const;

	// Swap the world at runtime
	void set_world(std::shared_ptr<hittable_list> new_world) { _world = std::move(new_world); }

	std::uint32_t get_num_of_samples() const { return _numOfSamples_rendered; }

private:
	void render();
	void render_mulpile_blocks();
	void render_multiple_samples();
	void render_square(int thread_idx, int left, int up, int height, int width);
	void sync_render_parameters();

	int	   _numOfBounces = 16;
	int _pixels_height_per_thread = 10;
	int _pixels_width_per_thread = 10;
	int	   _numOfThreads = 4;
	bool	_is_accumulation = false;

	int	   _numOfBounces_last = _numOfBounces;
	int _pixels_height_per_thread_last = _pixels_height_per_thread;
	int _pixels_width_per_thread_last = _pixels_width_per_thread;
	int	   _numOfThreads_last = _numOfThreads;
	bool	   _is_accumulation_last = _is_accumulation;
	bool   _is_rendering_multiple_samples = false;
	int	   _numOfSamples_rendered = 0;

	std::shared_ptr<camera>		   _camera;
	std::shared_ptr<hittable_list> _world;
	std::unique_ptr<ThreadPool>	   _pool;
	std::future<void>			   _future;
	render_callback				   _output_callback;
	bool						   _is_rendering = false;

	// For output samples image
	std::vector<std::vector<color>> _samples_images;
	std::vector<color>				_output_sample_image;
	std::vector<std::future<void>>	_futures;
	std::mutex						_mutex;
};