#pragma once

// OpenGL Extension Wrangler - declares functions used to find OpenGL function declarations
#include <GL/glew.h>

// GLFW - declares functions used for the GLFW window manager
#include <GLFW/glfw3.h>

// ImGui - declares functions used for the ImGui user interface
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// used to time events (currently only the main loop is timed)
#include <chrono>

extern float* output_image_ptr;
extern int	  resolution;
extern float  frame_seconds;
extern int	  num_of_samples_rendered;
extern bool	  is_accumulation;
extern int	  num_of_threads;
extern int	  num_of_bounces;
extern int	  pixels_width_per_thread;
extern int	  pixels_height_per_thread;
extern int   selected_scene;           // 0=default, 1=scene1, 2=scene2
extern bool  request_scene_reload;     // set true in GUI to rebuild scene at runtime

void ImGuiRender();
void DrawOutputImage();
void UpdateOutputTexture();