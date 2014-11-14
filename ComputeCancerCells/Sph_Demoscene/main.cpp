#include "App.hpp"
#undef main

int main(int argc, char **argv)
{
	App app;
	app.init();
	app.generateBuffers(128 * 128 );
	app.generateBuffers();
	while (app.run())
	{ }
	app.deactivate();
	return EXIT_SUCCESS;
}