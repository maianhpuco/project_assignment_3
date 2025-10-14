#include "helloworld.h"
#include "interval.h"

#include <thread>

/*
 * This function is a starting point for creating your own user interface. I just create a UI window and
 * add a timer. Other elements can be added by putting them between ImGui::Begin() and ImGui::End()
 */
void DrawInterfaceWindow()
{
	// Create a new ImGui window to show the image - call it whatever you want
	ImGui::Begin("Your Interface");

	// Display the render time for a single pass through the main loop
	ImGui::Text("Resolution: %d x %d", resolution, resolution);
	ImGui::Text("Render Time: %fms", frame_seconds * 1000);
	// std::thread::hardware_concurrency() reports how many hardware threads the CPU exposes.
	// We use it as an upper bound for the thread pool so we don't oversubscribe cores.
	ImGui::Text("Max threads: %d", std::thread::hardware_concurrency());
	ImGui::Text("Samples Rendered: %d", num_of_samples_rendered);
	ImGui::Text("Is Accumulation: %s", is_accumulation ? "True" : "False");

	// Scene selector
	// const char* scenes[] = {"Default", "One+ThreeLights", "Rings+LightRing"};
	// ImGui::Text("Scene:");
	// static int current = 0;
	// current = selected_scene;
	// if (ImGui::Combo("##Scene", &current, scenes, IM_ARRAYSIZE(scenes)))
	// {
	// 	selected_scene = current;
	// 	request_scene_reload = true;
	// }
	selected_scene = 2;
	ImGui::Text("Scene: Ring Light Rig");

	// Add buttons to change accumulation mode
	ImGui::Button("Accumulate Samples");
	if (ImGui::IsItemClicked())
	{
		is_accumulation = !is_accumulation;
	}

	// Add buttons to change number of threads
	ImGui::Text("Number of Threads: %d", num_of_threads);
	// Add textbox to type in number of threads
	char buf[10];
	snprintf(buf, 10, "%d", num_of_threads);
	ImGui::InputText("##Threads", buf, 10);
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		int n = atoi(buf);
		num_of_threads = n > 0 ? n : 1;
	}

	// Add buttons to change number of bounces
	ImGui::Text("Number of Bounces: %d", num_of_bounces);
	// Add textbox to type in number of bounces
	char buf2[10];
	snprintf(buf2, 10, "%d", num_of_bounces);
	ImGui::InputText("##Bounces", buf2, 10);
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		int n = atoi(buf2);
		if (n > 0)
		{
			num_of_bounces = n;
		}
	}

	// Add buttons to change render ratio
	ImGui::Text("Pixels per thread (width x height): %d x %d", pixels_width_per_thread, pixels_height_per_thread);
	// Add textbox to type in render ratio
	char buf3_width[10];
	snprintf(buf3_width, 10, "%d", pixels_width_per_thread);
	ImGui::InputText("##Pixels of width per thread", buf3_width, 10);
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		int r = atoi(buf3_width);
		if (r > 0 && r <= resolution)
		{
			pixels_width_per_thread = r;
		}
	}

	char buf3_height[10];
	snprintf(buf3_height, 10, "%d", pixels_height_per_thread);
	ImGui::InputText("##Pixels of height per thread", buf3_height, 10);
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		int r = atoi(buf3_height);
		if (r > 0 && r <= resolution)
		{
			pixels_height_per_thread = r;
		}
	}

	// This is the only thing displayed in the window
	ImGui::End();
}

/*
* This function renders the user interface. I'm actually cheating a little bit here: the only user
* interface window that's rendered is a "demo" that comes with the ImGui library. It basically has a
* bunch of widgets that show what ImGui is capable of, so you have some interesting stuff to play with
* and I didn't actually have to program any of it.
* 
* In any case, you can add your own user interface elements here.
*/
void ImGuiRender()
{
	// These functions initialize the UI rendering process with both OpenGL and GLFW
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	// This function creates a new "Frame", which is the basic foundation of an ImGui UI
	ImGui::NewFrame();
	{
		/* This renders an ImGui "Demo" window that shows off its UI elements (you can delete this and replace it with your own)
		 * You can get an equivalent of this window online, which also provides the code necessary to create each UI element:
		 * https://pthom.github.io/imgui_manual_online/manual/imgui_manual.html
		 */
		ImGui::ShowDemoWindow();

		// This renders an ImGui window displaying the output image
		DrawOutputImage();

		// Draw a placeholder user interface window (you can use this function or add additional windows with similar functions)
		DrawInterfaceWindow();
	}

	// This function makes the graphics API calls (in this case OpenGL) to render the user interface
	ImGui::Render();

	// This actually copies the GUI to the OpenGL frame buffer (in this case probably the GLFW back buffer)
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
