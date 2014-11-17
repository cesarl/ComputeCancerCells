#include "App.hpp"
#include <cassert>
#include <mutex>

#include <GL\glew.h>
#include <GL/glut.h>
#include <GL\GLU.h>
#include <GL\glext.h>
#include <GL\GL.h>

#include "ImguiConfig.hpp"
#include "Globals.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>

#include <vector>

static const glm::vec4 CANCER(1,0,0,1);
static const glm::vec4 HEALTH(0,1,0,1);
static const glm::vec4 MEDECINE(1,1,0,1);
static const glm::vec4 NONE(0,0,0,1);

App::App()
	: _window(nullptr)
	, _computeNewStateShader(nullptr)
	, _copyOldStateShader(nullptr)
	, _renderShader(nullptr)
	, _totalTime(0.0f)
	, _deltaTime(0)
	, _pastTime(0)
	, _width(WINDOW_W)
	, _height(WINDOW_H)
	, _workGroupSize(128)
	, _read(SboChannel::State1)
	, _write(SboChannel::State2)
	, _inject(false)
	, _cancerPercent(30)
	, _healthyPercent(30)
{
	for (auto i = 0; i < SboChannel::END; ++i)
		_sbos[i] = 0;
}

App::~App()
{
	deactivate();
}


void App::init()
{
	static std::once_flag flag;
	std::call_once(flag, [this](){
		assert(SDL_Init(SDL_INIT_VIDEO) == 0);

		_window = SDL_CreateWindow("COMPUTE SHADER CANCER CELLS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			_width, _height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
		assert(_window != nullptr);
		_context = SDL_GL_CreateContext(_window);
		glewInit();
		loadShaders();
		ImguiConf::InitImGui();
	});

}

void App::loadShaders()
{
	_computeNewStateShader = std::make_unique<OpenGLTools::Shader>("Shaders/ComputeNewState.kernel");
	_copyOldStateShader = std::make_unique<OpenGLTools::Shader>("Shaders/CopyOldState.kernel");
	_renderShader = std::make_unique<OpenGLTools::Shader>("Shaders/Render.vp", "Shaders/Render.fp");
}

void App::generateBuffers(GLuint particleNumber)
{
	assert(particleNumber != 0);
	_clean();

	glGenBuffers(SboChannel::END, _sbos);
}

void App::generateBuffers()
{
	assert(_sbos[0] != 0);

	// READ
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _sbos[_read]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, _width * _height * sizeof(glm::vec4), NULL, GL_STREAM_DRAW);

		GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

		glm::vec4 *points = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, _width * _height * sizeof(glm::vec4), bufMask);

		for (GLuint i = 0; i < _width * _height; ++i)
		{
		}
		unsigned int c = _width * _height * _cancerPercent / 100;
		unsigned int h = _width * _height * _healthyPercent / 100;
		unsigned int t = _width * _height;
		for (GLuint i = 0; i < _width * _height; ++i)
		{
			points[i] = glm::vec4(NONE.x, NONE.y, 0, 1);
		}

		while (c != 0)
		{
			points[rand() % _width + rand() % _height * _width] = glm::vec4(CANCER.x, CANCER.y, 0,0);
			c--;
		}
		while (h != 0)
		{
			points[rand() % _width + rand() % _height * _width] = glm::vec4(HEALTH.x, HEALTH.y, 0,0);
			h--;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// WRITE
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _sbos[_write]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, _width * _height * sizeof(glm::vec4), NULL, GL_STREAM_DRAW);

		GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

		glm::vec4 *points = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, _width * _height * sizeof(glm::vec4), bufMask);

		for (GLuint i = 0; i < _width * _height; ++i)
		{
			points[i] = NONE;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// POSITIONS
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _sbos[SboChannel::Positions]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, _width * _height * sizeof(glm::vec4), NULL, GL_STREAM_DRAW);

		GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

		glm::vec4 *points = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, _width * _height * sizeof(glm::vec4), bufMask);

		glm::vec4 div(_width / 2.0f, _height / 2.0f, 1, 1);
		for (GLuint i = 0; i < _width * _height; ++i)
		{
			points[i] = (glm::vec4(i % _width, i / _width, 1, 2) - div) / div;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// COUNTER
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _sbos[SboChannel::Counter]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(unsigned int), NULL, GL_STREAM_DRAW);

		GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

		unsigned int *points = (unsigned int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(unsigned int), bufMask);

		for (GLuint i = 0; i < 4; ++i)
		{
			points[i] = 0;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

bool App::run()
{
	if (!_updateInput())
		return false;
	ImguiConf::UpdateImGui();
	glEnable(GL_DEPTH_TEST);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// COPY OLD STATE
	{
		// CLEAR COUNTER
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, _sbos[Counter]);
			GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
			unsigned int *points = (unsigned int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(unsigned int), bufMask);
			for (std::size_t i = 0; i < 4; ++i)
				points[i] = 0;
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

		if (_inject)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, _sbos[_read]);

			GLint bufMask = GL_MAP_WRITE_BIT;
			glm::vec4 *points = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, _width * _height * sizeof(glm::vec4), bufMask);
			points[(_injectCoord.x) + (_height -_injectCoord.y) * _width] = glm::vec4(MEDECINE.x, MEDECINE.y, 0,1);
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			_inject = false;
		}

		auto *shader = _copyOldStateShader.get();
		auto shaderId = shader->getId();
		shader->use();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _sbos[_read]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _sbos[_write]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _sbos[Counter]);

		glDispatchCompute(_width * _height / _workGroupSize, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	}
	// COMPUTE NEW STATE
	{
		auto *shader = _computeNewStateShader.get();
		auto shaderId = shader->getId();
		shader->use();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _sbos[_read]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _sbos[_write]);

		auto width = glGetUniformLocation(shaderId, "Width");
		auto height = glGetUniformLocation(shaderId, "Height");

		glUniform1ui(width, _width);
		glUniform1ui(height, _height);

		glDispatchCompute(_width * _height / _workGroupSize, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
	}

	// RENDER
	{
		auto *shader = _renderShader.get();
		auto shaderId = shader->getId();
		shader->use();

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, _sbos[SboChannel::Positions]);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBindBuffer(GL_ARRAY_BUFFER, _sbos[_write]);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glDrawArrays(GL_POINTS, 0, _width * _height);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glUseProgram(0);
	}

	std::swap(_read, _write);

	///////////////////////
	/// GUI

	// FPS
	static float ms_per_frame[120] = { 0 };
	static int ms_per_frame_idx = 0;
	static float ms_per_frame_accum = 0.0f;
	ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
	ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
	ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
	ms_per_frame_idx = (ms_per_frame_idx + 1) % 120;
	const float ms_per_frame_avg = ms_per_frame_accum / 120;
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);


	if (ImGui::SliderInt("Cancer %", &_cancerPercent, 0, 100 - _healthyPercent))
	{
		generateBuffers();
	}
	if (ImGui::SliderInt("Healthy %", &_healthyPercent, 0, 100 - _cancerPercent))
	{
		generateBuffers();
	}

	// Display counter
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _sbos[Counter]);
		GLint bufMask = GL_MAP_READ_BIT;
		unsigned int *points = (unsigned int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(unsigned int), bufMask);
		ImGui::Text("Healthy cells : %i", points[0]);
		ImGui::Text("Cancer cells : %i", points[1]);
		ImGui::Text("Medecine cells : %i", points[2]);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	ImGui::Render();
	SDL_GL_SwapWindow(_window);

	return true;
}

bool App::deactivate()
{
	static std::once_flag flag;
	std::call_once(flag, [this](){
		_clean();
		_computeNewStateShader.release();
		_copyOldStateShader.release();
		_renderShader.release();
		ImGui::Shutdown();
		SDL_GL_DeleteContext(_context);
		SDL_DestroyWindow(_window);
		SDL_Quit();
	});
	return true;
}

bool App::_updateInput()
{
	SDL_Event event;

	auto dtNow = SDL_GetTicks();
	if (dtNow > _pastTime)
	{
		_deltaTime = (float)(dtNow - _pastTime) / 1000.0f;
		_pastTime = dtNow;
	}

	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
		{
			_inject = true;
			_injectCoord = glm::uvec2(event.motion.x, event.motion.y);
		}
		else if (event.type == SDL_MOUSEMOTION)
		{
			_injectCoord = glm::uvec2(event.motion.x, event.motion.y);
		}
		else if (event.type == SDL_KEYDOWN)
		{
		}
		else
		{
			if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				return false;
			}
			else if (event.key.keysym.sym == SDLK_r) // reloadShader
			{
				loadShaders();
			}
			else if (event.key.keysym.sym == SDLK_SPACE) // reloadShader
			{
				generateBuffers();
			}
		}
	}
	return true;
}

void App::_clean()
{
	if (_sbos[0] != 0)
		glDeleteBuffers(SboChannel::END, _sbos);
	for (auto i = 0; i < SboChannel::END; ++i)
		_sbos[i] = 0;
}