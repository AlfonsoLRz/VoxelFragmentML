#include "stdafx.h"

#include "Graphics/Application/CADScene.h"
#include "Graphics/Application/Renderer.h"
#include "Graphics/Core/FragmentationProcedure.h"
#include "Interface/Window.h"
#include <windows.h>						// DWORD is undefined otherwise

#define DATASET_GENERATION false

// Laptop support. Use NVIDIA graphic card instead of Intel
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


int main(int argc, char *argv[])
{
	srand(time(nullptr));
	
	std::cout << "__ Starting fragmentation __" << std::endl;

	const std::string title = "Vessel fragmentation";
	const uint16_t width = 1050, height = 650;
	const auto window = Window::getInstance();
	
	{
		if (const bool success = window->load(title, width, height))
		{
#if !DATASET_GENERATION
			Renderer::getInstance()->getCurrentScene()->load();
			window->startRenderingCycle();
#else
			FragmentationProcedure procedure;
			CADScene* scene = dynamic_cast<CADScene*>(Renderer::getInstance()->getCurrentScene());
			scene->generateDataset(procedure, procedure._folder, procedure._searchExtension, procedure._destinationFolder);
#endif

			std::cout << "__ Finishing LiDAR Simulator __" << std::endl;
		}
		else
		{
			std::cout << "__ Failed to initialize GLFW __" << std::endl;
		}
	}

	system("pause");

	return 0;
}
