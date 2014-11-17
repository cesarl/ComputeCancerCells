#pragma once

#include "Shader.hpp"
#include <SDL.h>
#include <memory>
#include <glm/glm.hpp>

enum SboChannel
{
	State1 = 0
	, State2
	, Positions
	, Counter
	, END // keep at the end
};

class App
{
public:
	App();
	~App();
	App(const App &) = delete;
	App(App &&) = delete;
	App &operator=(const App &) = delete;
	App &operator=(App &&) = delete;
	void init();
	void generateBuffers(GLuint particleNumber);
	void generateBuffers();
	bool run();
	bool deactivate();
	void loadShaders();
	bool _updateInput();
private:
	std::unique_ptr<OpenGLTools::Shader> _computeNewStateShader;
	std::unique_ptr<OpenGLTools::Shader> _copyOldStateShader;
	std::unique_ptr<OpenGLTools::Shader> _renderShader;
	SDL_Window *_window;
	unsigned int _width;
	unsigned int _height;
	GLuint _workGroupSize;
	SDL_GLContext _context;
	GLuint _sbos[SboChannel::END];
	float _totalTime;
	float _deltaTime;
	std::size_t _pastTime;
	SboChannel _read;
	SboChannel _write;
	bool _inject;
	glm::uvec2 _injectCoord;
	int _cancerPercent;
	int _healthyPercent;
	float _zoom;

	void _clean();
};