/*
	This is a demo program designed to teach you how to compile an application that uses
	multiple libraries:

	1) OpenGL is a graphics application programming interface (API) designed to interact
		directly with graphics cards to draw three-dimensional stuff on the screen.
		https://www.opengl.org/

	2) GLFW is a "window manager" specifically designed for OpenGL. It works with the
		operating system to create a window that acts as an OpenGL "context", which is
		a part of the screen that acts as a target for OpenGL commands.
		https://www.glfw.org/

	3) ImGui is a real-time graphical user interface library. This uses a graphics API
		like OpenGL to render standard GUI objects like buttons, menus, list boxes, etc.
		The GitHub page is worth looking at: https://github.com/ocornut/imgui
		Here is a web-based demo that shows you the code needed to create any UI element:
		https://pthom.github.io/imgui_manual_online/manual/imgui_manual.html

	4) GLM is a light-weight linear algebra library that defines C/C++ objects useful for
		2D and 3D graphics.
		https://github.com/g-truc/glm
*/

// This header files contains declarations for functions and variables that are used across multiple C files.
#include "camera.h"
#include "helloworld.h"
#include "material.h"
#include "renderer.h"
#include "sphere.h"
#include "vec3.h"

// I include this file to throw runtime errors, but you can also use it to output debugging information.
#include <future>
#include <iostream>
#include <cmath>

int	   resolution = 500;			// resolution of the output image (you can add a user interface element to change this)
float* output_image_ptr = nullptr;	// pointer to the output image data (if you change the resolution make sure to change this!)
float  frame_seconds = 0.0f;		// time it takes to go through the main "game" loop (directly translates to frame rate or fps)
int	   num_of_samples_rendered = 1;
bool   is_accumulation = true;		   // whether or not to accumulate samples over multiple frames (you can add a user interface element to change this)
int	   num_of_threads = 10;		   // number of threads to use for rendering (you can add a user interface element to change this)
int	   pixels_width_per_thread = resolution / num_of_threads;   // number of pixels in the horizontal direction to render per thread (you can add a user interface element to change this)
int	   pixels_height_per_thread = resolution / num_of_threads;  // number of pixels in the vertical direction to render per thread (you can add a user interface element to change this)
// int	   pixels_width_per_thread = 50;   // number of pixels in the horizontal direction to render per thread (you can add a user interface element to change this)
// int	   pixels_height_per_thread = 50; 
int	   num_of_bounces = 5;			   // number of bounces for each ray (you can add a user interface element to change this)
int	   selected_scene = 0;        // 0=default grid, 1=One+ThreeLights, 2=Rings+LightRing
bool   request_scene_reload = false; // set by GUI to request scene rebuild

int main(int argc, const char* argv[])
{
	/*
	* GLFW is the window manager that we will be using. Its job is to create
	* an "OpenGL context", which is a region of the screen that is used as
	* a target for OpenGL commands. This function initializes the GLFW library
	* and throws an exception if there are any hardware or software issues that
	* prevent it from initializing.
	*/
	if (!glfwInit())
		throw std::runtime_error("Failed to initialize GLFW");

	/*
	* This function call uses GLFW to create a window, including an OpenGL context. The
	* GLFWwindow pointer will be used in any GLFW library calls that effect this particular
	* window. For example, if you want to assign a callback function that responds to keyboard
	* or mouse input, you'll pass the GLFWwindow pointer. This also allows you to generate
	* multiple windows - you just make another call to glfwCreateWindow() and store the window
	* in a different GLFWwindow pointer! The next line of code makes sure the window is
	* successfully created (and throws a runtime exception if something went wrong).
	*/
	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Hello World", nullptr, nullptr);
	if (window == nullptr)
		throw std::runtime_error("Failed to create GLFW window");

	/*
	* This function makes the OpenGL context assigned to the window we created the "current"
	* context. That means that any OpenGL calls that we make will be sent to this context. If
	* we had multiple windows, we would have to set the contexts to the appropriate window
	* before we started sending commands to draw stuff.
	*/
	glfwMakeContextCurrent(window);

	/*
	* Now we're going to initialize the ImGui library. This is a really cool library that
	* uses OpenGL (or any graphics API) to render standard graphical user interface components.
	*/
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// This sets the GUI style to one that I personally think looks cool
	ImGui::StyleColorsDark();

	/*
	* These functions tell ImGui to use OpenGL and GLFW to render the user interface
	* and also specifies the version of the OpenGL shading language (GLSL) to use
	* for rendering. We will discuss shading languages near the end of the class since this
	* is the foundation for GPU computing. For now, all you need to know is that it's a
	* sub-language that is part of OpenGL and used to render stuff to individual pixels in the window.
	*/
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 120";
	ImGui_ImplOpenGL3_Init(glsl_version);

	/*
	* This initializes a library called the OpenGL Extension Wrangler (GLEW) library. This is mostly
	* used for Windows and is a side effect of how the OpenGL library works with the operating system.
	* Basically, by default Windows only provides access to a very early version of OpenGL. This library
	* "mines" the OpenGL libraries to figure out what extensions are ACTUALLY supported by your graphics
	* driver. Don't worry too much about this - you'll probably never use anything beyond these two lines
	* of code.
	*/
	if (glewInit() != GLEW_OK)
		throw std::runtime_error("Failed to initialize GLEW");

	/*
	 * Allocate space on the heap to store the image that will be displayed on the screen.
	 */
	output_image_ptr = new float[resolution * resolution * 4];

	/*
	 * This function creates a placeholder image so that you see a result on the screen the first time you run it.
	 * You should see an "RGB square" where the red and blue channels change along the x-axis and the green channel
	 * changes along the y-axis.
	 */

	auto cam = std::make_shared<camera>();

	cam->aspect_ratio = 1.0;
	cam->image_width = resolution;

	// cam->vfov = 20;
	// cam->lookfrom = point3(13, 2, 3);
	// cam->lookat = point3(0, 0, 0);
	// cam->vup = vec3(0, 1, 0);
	cam->vfov = 35;
	cam->lookfrom = point3(0, 5, 14);
	cam->lookat = point3(0, 1.0, 0);
	cam->vup = vec3(0, 1, 0); 
	
	cam->init();
	auto world = std::make_shared<hittable_list>();

	auto output_callback =
			[](std::size_t idx, color& color, float transparent) {
				output_image_ptr[idx] = color.x();
				output_image_ptr[idx + 1] = color.y();
				output_image_ptr[idx + 2] = color.z();
				output_image_ptr[idx + 3] = transparent;
			};

	// Ring of spheres on ground, and a ring of emissive spheres above
	auto ground_mat = std::make_shared<path_tracer_material>(color(0.6f, 0.6f, 0.6f), 0.0f);
	world->add(std::make_shared<sphere>(point3(0, -1000, 0), 1000.0, ground_mat));

	const int	 N = 12;
	// const float R = 2.397f; // Previous tight spacing left almost no visible gap.
	const float R = 2.55f;	   // Increase separation so the balls sit farther apart.
	const float rObj = 0.62f;
	const float objectY = rObj; // sit directly on ground plane (y = 0)
	const float lightY = 6.0f;
	const float rLight = 0.5f;
	const float E = 30.0f;

	// Diffuse ring
	for (int i = 0; i < N; ++i)
	{
		float a = 2.0f * static_cast<float>(pi) * static_cast<float>(i) / static_cast<float>(N);
		float x = R * std::cos(a);
		float z = R * std::sin(a);

		auto mat = std::make_shared<path_tracer_material>(color(0.8f, 0.8f, 0.8f), 0.0f);
		world->add(std::make_shared<sphere>(point3(x, objectY, z), rObj, mat));
	}

	// Emissive ring
	for (int i = 0; i < N; ++i)
	{
		float a = 2.0f * static_cast<float>(pi) * static_cast<float>(i) / static_cast<float>(N);
		float x = R * std::cos(a);
		float z = R * std::sin(a);

		// Previous hue wheel used equal weights, yielding magenta-heavy spill
		// color emission_color(0.5f + 0.5f * std::cos(a + 0.0f),
		// 					 0.5f + 0.5f * std::cos(a + 2.094f),
		// 					 0.5f + 0.5f * std::cos(a + 4.188f));
		// color emission_color(0.45f + 0.35f * std::cos(a + 0.0f),
		// 					 0.55f - 0.35f * std::cos(a + 2.094f),
		// 					 0.30f + 0.20f * std::cos(a + 4.188f));
		color emission_color(0.55f + 0.35f * std::cos(a + 0.0f),
							 0.35f + 0.20f * std::cos(a + 2.094f),
							 0.55f + 0.35f * std::cos(a + 4.188f));

		auto light_mat = std::make_shared<path_tracer_material>(color(1.0f, 1.0f, 1.0f), E, emission_color);
		auto light_sphere = std::make_shared<sphere>(point3(x, lightY, z), rLight, light_mat);
		world->add_light(light_sphere);
	}

	renderer render_engine(
			cam,
			world,
			output_callback);
	render_engine.init(num_of_threads,
		num_of_bounces, pixels_height_per_thread, pixels_width_per_thread, num_of_threads, is_accumulation
	);
	render_engine.start_rendering();
	/*
	* This is what we call the "main rendering loop" (sometimes in gaming you'll call it the "main game loop".
	* This is where everything in the program happens. Every iteration through the loop re-draws everything
	* onto the window (including the user interface). It also queries sources for user input (mouse,
	* keyboard, etc.) and updates variables based on callback functions that you define. This loop runs
	* continuously until you kill the program (ex. by pressing the X button to close the window).
	*/
	while (!glfwWindowShouldClose(window))
	{
		auto start = std::chrono::high_resolution_clock::now();

		// This function checks potential input sources (keyboard, mouse, etc.) and executes callback functions
		glfwPollEvents();
		// ESC key to close the window
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		// This function tells OpenGL to clear the window (in this case it writes the color "black" to all pixels)
		glClear(GL_COLOR_BUFFER_BIT);

		// This function is defined in the helloworld_gui.cpp file and renders the user interface
		ImGuiRender();

		/*
		* This function takes advantage of a concept called "double buffering". There is a region
		* of memory on your graphics card that displays the contents of the screen. The monitor reads this region
		* of memory every time it refreshes. This introduces a problem: if you draw to this area of memory
		* the user may be able to see the individual objects that you draw, resulting in "flickering". Double
		* buffering prevents this by sending the draw calls to a "back buffer" that isn't displayed on the
		* computer monitor. This function switches the memory pointers so that the information on the monitor
		* is updated immediately.
		*/
		glfwSwapBuffers(window);  // swap the double buffer

		auto						 end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> duration = end - start;
		frame_seconds = duration.count();
		num_of_samples_rendered = render_engine.get_num_of_samples();
		render_engine.set_accumulation(is_accumulation);
		render_engine.change_num_of_threads(num_of_threads);
		render_engine.change_num_of_bounces(num_of_bounces);
		render_engine.change_render_ratio(pixels_width_per_thread, pixels_height_per_thread);

		is_accumulation = render_engine.get_accumulation();
	}

	render_engine.stop_rendering();

	/*
	* All of these functions just destroy the stuff that we've created to make sure that there aren't any
	* memory leaks.
	*/
	ImGui_ImplOpenGL3_Shutdown();  // Shut down ImGui's connection with OpenGL
	ImGui_ImplGlfw_Shutdown();	   // Shut down ImGui's connection with GLFW
	ImGui::DestroyContext();	   // Clear the ImGui user interface
	glfwDestroyWindow(window);	   // Destroy the GLFW rendering window
	glfwTerminate();			   // Terminate GLFW
}
