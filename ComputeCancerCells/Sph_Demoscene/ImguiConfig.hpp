#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#include <gl/GL.h>
#define STB_IMAGE_IMPLEMENTATION
#include <imgui/stb_image.h>
#include <imgui/imgui.h>
#include <chrono>
#include "Globals.hpp"

namespace ImguiConf
{

	static GLuint fontTex;	

	static void ImImpl_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
	{
		if (cmd_lists_count == 0)
			return;

		// We are using the OpenGL fixed pipeline to make the example code simpler to read!
		// A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
		// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		// Setup texture
		glBindTexture(GL_TEXTURE_2D, fontTex);
		glEnable(GL_TEXTURE_2D);

		// Setup orthographic projection matrix
		const float width = ImGui::GetIO().DisplaySize.x;
		const float height = ImGui::GetIO().DisplaySize.y;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// Render command lists
		for (int n = 0; n < cmd_lists_count; n++)
		{
			const ImDrawList* cmd_list = cmd_lists[n];
			const unsigned char* vtx_buffer = (const unsigned char*)cmd_list->vtx_buffer.begin();
			glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer));
			glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + 8));
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + 16));

			int vtx_offset = 0;
			const ImDrawCmd* pcmd_end = cmd_list->commands.end();
			for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
			{
				glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
				glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
				vtx_offset += pcmd->vtx_count;
			}
		}
		glDisable(GL_SCISSOR_TEST);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisable(GL_BLEND);
	}

	static void scroll_callback(double xoffset, double yoffset)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseWheel = (yoffset != 0.0f) ? yoffset > 0.0f ? 1 : -1 : 0;           // Mouse wheel: -1,0,+1
	}

	static void key_callback(int key, int action)
	{
		//ImGuiIO& io = ImGui::GetIO();
		//if (action == GLUT_KEY_DOWN)
		//	io.KeysDown[key] = true;
		//if (action == GLUT_KEY_UP)
		//	io.KeysDown[key] = false;
		//io.KeyCtrl = glutGetModifiers() == GLUT_ACTIVE_CTRL;
		//io.KeyShift = glutGetModifiers() == GLUT_ACTIVE_SHIFT;
	}

	static void char_callback(unsigned int c)
	{
		if (c > 0 && c <= 255)
			ImGui::GetIO().AddInputCharacter((char)c);
	}

	void InitImGui()
	{
		int w = WINDOW_W, h = WINDOW_H;

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)w, (float)h);        // Display size, in pixels. For clamping windows positions.
		io.DeltaTime = 1.0f / 60.0f;                          // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
		io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels

		io.RenderDrawListsFn = ImImpl_RenderDrawLists;

		// Load font texture
		glGenTextures(1, &fontTex);
		glBindTexture(GL_TEXTURE_2D, fontTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		const void* png_data;
		unsigned int png_size;
		ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
		int tex_x, tex_y, tex_comp;
		void* tex_data = stbi_load_from_memory((const unsigned char*)png_data, (int)png_size, &tex_x, &tex_y, &tex_comp, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_x, tex_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
		stbi_image_free(tex_data);
	}

	void UpdateImGui()
	{
		ImGuiIO& io = ImGui::GetIO();

		static double time = 0.0f;
		const double current_time = SDL_GetTicks();
		float dif = (float)(current_time - time);
		io.DeltaTime = dif == 0.0f ? 0.000000001f : dif / 1000.0f;
		time = current_time;

		int mx, my;
		auto mouseState = SDL_GetMouseState(&mx, &my);
		io.MousePos = ImVec2((float)mx, (float)my);
		io.MouseDown[0] = (bool)(mouseState & SDL_BUTTON(SDL_BUTTON_LEFT));
		io.MouseDown[1] = (bool)(mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT));
		io.MouseDown[2] = (bool)(mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE));

		// Start the frame
		ImGui::NewFrame();
	}
}