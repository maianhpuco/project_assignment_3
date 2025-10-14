#include "renderer.h"

#include "helloworld.h"
#include "material.h"

#include <algorithm>
renderer::renderer(const renderer& other)
	  : _camera(other._camera), _world(other._world), _pool(std::make_unique<ThreadPool>(other._numOfThreads)), _output_callback(other._output_callback), _numOfBounces(other._numOfBounces), _pixels_height_per_thread(other._pixels_height_per_thread), _pixels_width_per_thread(other._pixels_width_per_thread), _numOfThreads(other._numOfThreads), _numOfBounces_last(other._numOfBounces_last), _pixels_height_per_thread_last(other._pixels_height_per_thread_last), _pixels_width_per_thread_last(other._pixels_width_per_thread_last), _numOfThreads_last(other._numOfThreads_last)
{
}

renderer& renderer::operator=(const renderer& other)
{
	if (this != &other)
	{
		_camera = other._camera;
		_world = other._world;
		_pool = std::make_unique<ThreadPool>(other._numOfThreads);
		_output_callback = other._output_callback;
		_numOfBounces = other._numOfBounces;
		_pixels_height_per_thread = other._pixels_height_per_thread;
		_pixels_width_per_thread = other._pixels_width_per_thread;
		_numOfThreads = other._numOfThreads;
		_numOfBounces_last = other._numOfBounces_last;
		_pixels_height_per_thread_last = other._pixels_height_per_thread_last;
		_pixels_width_per_thread_last = other._pixels_width_per_thread_last;
		_numOfThreads_last = other._numOfThreads_last;
	}
	return *this;
}

renderer::~renderer()
{
	if (_is_rendering)
	{
		stop_rendering();
	}
}

renderer::renderer(std::shared_ptr<camera> cam, std::shared_ptr<hittable_list> world, const render_callback& callback)
	  : _camera(cam), _world(world), _pool(std::make_unique<ThreadPool>(_numOfThreads)), _output_callback(callback)
{
	_pool->init();
}

void renderer::init(int num_of_threads, int numOfBounces, int pixels_height_per_thread, int pixels_width_per_thread, int numOfThreads, bool is_accumulation)
{
	_is_accumulation_last = is_accumulation;
	_numOfBounces_last = numOfBounces;
	_pixels_width_per_thread_last = pixels_width_per_thread;
	_pixels_height_per_thread_last = pixels_height_per_thread;
	_numOfThreads_last = num_of_threads;
	is_accumulation = _is_accumulation_last;
	_numOfBounces = _numOfBounces_last;
	_pixels_width_per_thread = _pixels_width_per_thread_last;
	_pixels_height_per_thread = _pixels_height_per_thread_last;
	_numOfThreads = _numOfThreads_last;

	_samples_images.resize(_numOfThreads, std::vector<color>(_camera->get_image_width() * _camera->get_image_height(), color{}));
	_output_sample_image.resize(_camera->get_image_width() * _camera->get_image_height(), color{});
}

void renderer::render()
{
	auto image_height = _camera->get_image_height();
	auto image_width = _camera->get_image_width();

	// Reset if not accummulation
	if (!_is_accumulation)
	{
		for (auto& image : _samples_images)
		{
			std::fill(image.begin(), image.end(), color{});
		}

		std::fill(_output_sample_image.begin(), _output_sample_image.end(), color{});
	}

	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_is_rendering)
		{
			return;
		}

		for (int yi = 0; yi < image_height;)
		{
			for (int xi = 0; xi < image_width;)
			{
				if (_pixels_height_per_thread == image_height && _pixels_width_per_thread == image_width)
				{ // Render multiple samples
					_is_rendering_multiple_samples = true;
					for (int thread_idx = 0; thread_idx < _numOfThreads; thread_idx++)
					{
						_futures.push_back(
								_pool->submit([this, thread_idx, xi, yi]() {
									this->render_square(thread_idx, xi, yi, _pixels_height_per_thread, _pixels_width_per_thread);
								}));
						if (_is_accumulation)
							_numOfSamples_rendered++;
						else
							_numOfSamples_rendered = 0;
					}
				}
				else
				{ // Render a square block multithreadedly
					_is_rendering_multiple_samples = false;
					// _futures.push_back(
					// 		_pool->submit([this, xi, yi]() {
					// 			this->render_square(0, xi, yi, _pixels_height_per_thread, _pixels_width_per_thread);
					// 		}));

					int block_width = std::min(_pixels_width_per_thread, image_width - xi);
					int block_height = std::min(_pixels_height_per_thread, image_height - yi);
					int tile_index = static_cast<int>(_futures.size());
					int thread_slot = tile_index % _numOfThreads;
					_futures.push_back(
							_pool->submit([this, thread_slot, xi, yi, block_height, block_width]() {
								this->render_square(thread_slot, xi, yi, block_height, block_width);
							}));
				}

				xi += _pixels_width_per_thread;
			}

			yi += _pixels_height_per_thread;
		}

	// Wait for all threads to finish
	for (const auto& future : _futures)
	{
		future.wait();
	}
	_futures.clear();
	}

	if (_is_rendering_multiple_samples)
	{
		// outut images
		for (int yi = 0; yi < image_height;yi++)
		{
			for (int xi = 0; xi < image_width;xi++)
			{
				auto idx = (yi * _camera->get_image_width() + xi);
				color color_sample = _output_sample_image[idx];
				for (int thread_idx = 0; thread_idx < _numOfThreads; thread_idx++)
				{
					std::lock_guard<std::mutex> lock(_mutex);
					color_sample += _samples_images[thread_idx][yi * _camera->get_image_width() + xi];
				}
				_output_sample_image[idx] = color_sample;
				auto output = color_sample / static_cast<float>(_numOfSamples_rendered);
				_output_callback(idx * 4, output, 1.0f);
			}
		}
	}
	else
	{
		// Make sure to update data before next render
		if (_is_accumulation)
			_numOfSamples_rendered++;
		else
			_numOfSamples_rendered = 0;
	}
}

void renderer::render_mulpile_blocks()
{

}

void renderer::start_rendering()
{
	if (_is_rendering)
		stop_rendering();
	_future = std::async([this] {
		_is_rendering = true;
		while (_is_rendering)
		{
			render();
		}
	});
}

void renderer::stop_rendering()
{
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_is_rendering = false;
		_pool->shutdown();
		for (auto& future : _futures)
		{
			future.wait();
		}
		_futures.clear();
	}
	_future.wait();
}

template <typename T>
bool is_same(T a, T b)
{
	return a == b;
}

void renderer::sync_render_parameters()
{
	bool is_any_change = false;
	if (!is_same(_is_accumulation, _is_accumulation_last))
	{
		is_any_change = true;
	}
	if (!is_same(_numOfBounces, _numOfBounces_last))
	{
		
		is_any_change = true;
	}

	if (!is_same(_pixels_height_per_thread, _pixels_height_per_thread_last))
	{
		is_any_change = true;
	}

	if (!is_same(_pixels_width_per_thread, _pixels_width_per_thread_last))
	{
		
		is_any_change = true;
	}

	if (!is_same(_numOfThreads, _numOfThreads_last))
	{
		
		is_any_change = true;
	}

	if (is_any_change)
	{
		stop_rendering();

		_is_accumulation = _is_accumulation_last;
		_numOfBounces = _numOfBounces_last;
		_pixels_width_per_thread = _pixels_width_per_thread_last;
		_pixels_height_per_thread = _pixels_height_per_thread_last;

		if (_pixels_width_per_thread == _camera->get_image_width() && _pixels_height_per_thread == _camera->get_image_height())
		{
			_is_accumulation_last = true;
			_is_accumulation = _is_accumulation_last;
		}

		_numOfThreads = _numOfThreads_last;
		_pool.reset(new ThreadPool(_numOfThreads));
		_pool->init();
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_samples_images.clear();
			_samples_images.resize(_numOfThreads,  std::vector<color>(_camera->get_image_height() * _camera->get_image_width(), color{}));
			_output_sample_image.clear();
			_output_sample_image.resize(_camera->get_image_width() * _camera->get_image_height(), color{});
		}
		memset(output_image_ptr, 0, sizeof(float) * _camera->get_image_height() * _camera->get_image_width() * 4);
		_numOfThreads = _numOfThreads_last;
		_numOfSamples_rendered = 0;

		// start rendering
		start_rendering();
	}
}

void renderer::change_num_of_bounces(int n)
{
	_numOfBounces_last = n;
	sync_render_parameters();
}

void renderer::change_render_ratio(int width, int height)
{
	_pixels_width_per_thread_last = width;
	_pixels_height_per_thread_last = height;
	sync_render_parameters();
}

void renderer::change_num_of_threads(int n)
{
	_numOfThreads_last = n;
	sync_render_parameters();
}

void renderer::set_accumulation(bool on)
{
	_is_accumulation_last = on;
	sync_render_parameters();
}

bool renderer::get_accumulation() const
{
	return _is_accumulation;
}

void renderer::resize_sample()
{
	_samples_images.resize(_numOfThreads, std::vector<color>(_camera->get_image_height() * _camera->get_image_width(), color{}));
}

void renderer::render_square(int thread_idx, int left, int up, int height, int width)
{
	for (int yi = up; yi < up + height; yi++)
	{
		for (int xi = left; xi < left + width; xi++)
		{
			if (!_is_rendering)
				return;

			auto r = _camera->generate_ray(xi, yi);
			color light = _camera->ray_color(r, *_world, _numOfBounces);

			if (!_is_rendering_multiple_samples)
			{
				_samples_images[thread_idx][yi * _camera->get_image_width() + xi] += light;
				auto color_sample = _samples_images[thread_idx][yi * _camera->get_image_width() + xi];
				color_sample /= static_cast<float>(_numOfSamples_rendered + 1);
				auto idx = (yi * _camera->get_image_width() + xi) * 4;
				// simple gamma 2.0 for display
				color_sample = color(std::sqrt(std::max(0.0, color_sample.x())),
								  std::sqrt(std::max(0.0, color_sample.y())),
								  std::sqrt(std::max(0.0, color_sample.z())));
				_output_callback(idx, color_sample, 1.0f);
			}
			else
			{
				_samples_images[thread_idx][yi * _camera->get_image_width() + xi] = light;
			}
		}
	}
}
