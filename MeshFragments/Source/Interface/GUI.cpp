#include "stdafx.h"
#include "GUI.h"

#include "Graphics/Application/Renderer.h"
#include "imfiledialog/ImGuiFileDialog.h"
#include "Interface/Fonts/font_awesome.hpp"
#include "Interface/Fonts/lato.hpp"
#include "Interface/Fonts/IconsFontAwesome5.h"

/// [Protected methods]

GUI::GUI() :
	_showRenderingSettings(false), _showSceneSettings(false), _showScreenshotSettings(false), _showAboutUs(false), _showControls(false), _showFractureSettings(false), _showFileDialog(false),
	_fractureText("")
{
	_renderer = Renderer::getInstance();
	_renderingParams = Renderer::getInstance()->getRenderingParameters();
	_scene = dynamic_cast<CADScene*>(_renderer->getCurrentScene());
	_fractureParameters = _scene->getFractureParameters();
}

void GUI::createMenu()
{
	ImGuiIO& io = ImGui::GetIO();

	if (_showRenderingSettings)		showRenderingSettings();
	if (_showScreenshotSettings)	showScreenshotSettings();
	if (_showFragmentList)			showFractureList();
	if (_showFractureSettings)		showFractureSettings();
	if (_showAboutUs)				showAboutUsWindow();
	if (_showControls)				showControls();
	if (_showFileDialog)			showFileDialog();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu(ICON_FA_COG "Settings"))
		{
			ImGui::MenuItem(ICON_FA_CUBE "Rendering", NULL, &_showRenderingSettings);
			ImGui::MenuItem(ICON_FA_IMAGE "Screenshot", NULL, &_showScreenshotSettings);
			ImGui::MenuItem(ICON_FA_TREE "Scene", NULL, &_showSceneSettings);
			ImGui::MenuItem(ICON_FA_HEART_BROKEN "Fracture", NULL, &_showFractureSettings);
			ImGui::MenuItem(ICON_FA_UTENSILS "Fracture List", NULL, &_showFragmentList);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FA_QUESTION_CIRCLE "Help"))
		{
			ImGui::MenuItem(ICON_FA_INFO "About the project", NULL, &_showAboutUs);
			ImGui::MenuItem(ICON_FA_GAMEPAD "Controls", NULL, &_showControls);
			ImGui::EndMenu();
		}

		ImGui::SameLine(io.DisplaySize.x - 300, 0);
		ImGui::Text(_fractureText.c_str());

		ImGui::SameLine(io.DisplaySize.x - 105, 0);
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
		ImGui::EndMainMenuBar();
	}
}

void GUI::leaveSpace(const unsigned numSlots)
{
	for (int i = 0; i < numSlots; ++i)
	{
		ImGui::Spacing();
	}
}

void GUI::renderHelpMarker(const char* message)
{
	ImGui::TextDisabled(ICON_FA_QUESTION);
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(message);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void GUI::renderText(const std::string& title, const std::string& content, char delimiter)
{
	std::string txt = title + (title.empty() ? "" : ": ") + content;
	ImGui::Text(txt.c_str());
}

void GUI::showAboutUsWindow()
{
	if (ImGui::Begin("About the project", &_showAboutUs))
	{
		ImGui::Text("This code belongs to a research project from University of Jaen (GGGJ group).");
	}

	ImGui::End();
}

void GUI::showControls()
{
	if (ImGui::Begin("Scene controls", &_showControls))
	{
		ImGui::Columns(2, "ControlColumns"); // 4-ways, with border
		ImGui::Separator();
		ImGui::Text("Movement"); ImGui::NextColumn();
		ImGui::Text("Control"); ImGui::NextColumn();
		ImGui::Separator();

		const int NUM_MOVEMENTS = 14;
		const char* movement[] = { "Orbit (XZ)", "Undo Orbit (XZ)", "Orbit (Y)", "Undo Orbit (Y)", "Dolly", "Truck", "Boom", "Crane", "Reset Camera", "Take Screenshot", "Continue Animation", "Zoom +/-", "Pan", "Tilt" };
		const char* controls[] = { "X", "Ctrl + X", "Y", "Ctrl + Y", "W, S", "D, A", "Up arrow", "Down arrow", "R", "K", "I", "Scroll wheel", "Move mouse horizontally(hold button)", "Move mouse vertically (hold button)" };

		for (int i = 0; i < NUM_MOVEMENTS; i++)
		{
			ImGui::Text(movement[i]); ImGui::NextColumn();
			ImGui::Text(controls[i]); ImGui::NextColumn();
		}

		ImGui::Columns(1);
		ImGui::Separator();

	}

	ImGui::End();
}

void GUI::showFileDialog()
{
	ImGuiFileDialog::Instance()->OpenDialog("Choose Point Cloud", "Choose File", ".obj", ".");

	// display
	if (ImGuiFileDialog::Instance()->Display("Choose Point Cloud"))
	{
		// Action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			_modelFilePath = filePathName.substr(0, filePathName.find_last_of("."));

			_fragmentMetadata.clear();
			_scene->fractureGrid(_modelFilePath, _fragmentMetadata, *_fractureParameters);
		}

		// Close
		ImGuiFileDialog::Instance()->Close();
		_showFileDialog = false;
	}
}

void GUI::showFractureList()
{
	if (_fragmentMetadata.empty()) _fragmentMetadata = _scene->getFragmentMetadata();

	ImGui::SetNextWindowSize(ImVec2(480, 440), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Fragments", &_showFragmentList, ImGuiWindowFlags_None))
	{
		this->leaveSpace(2);

		// Left
		static int selectedFragment = 0;

		ImGui::BeginChild("Fragments", ImVec2(200, 0), true);

		for (int i = 0; i < _fragmentMetadata.size(); ++i)
		{
			const std::string name = "Fragment " + std::to_string(i + 1);
			if (ImGui::Selectable(name.c_str(), selectedFragment == i))
				selectedFragment = i;
		}

		ImGui::EndChild();

		ImGui::SameLine();

		// Right
		ImGui::BeginGroup();
		ImGui::BeginChild("Fragment View", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));		// Leave room for 1 line below us

		const std::string name = "Fragment " + std::to_string(selectedFragment + 1);
		ImGui::Text(name.c_str());
		ImGui::Separator();

		this->leaveSpace(1);

		const FragmentationProcedure::FragmentMetadata& metadata = _fragmentMetadata[selectedFragment];
		this->renderText("File name: ", metadata._vesselName);
		this->renderText("Number of voxels: ", std::to_string(metadata._voxels));
		this->renderText("Number of voxels (percentage): ", std::to_string(metadata._percentage));
		this->renderText("Number of occupied voxels in vessel: ", std::to_string(metadata._occupiedVoxels));
		this->renderText("Voxelization size: ", std::to_string(metadata._voxelizationSize.x) + ", " + std::to_string(metadata._voxelizationSize.y) + ", " + std::to_string(metadata._voxelizationSize.z));

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	ImGui::End();
}

void GUI::showFractureSettings()
{
	if (ImGui::Begin("Fracture Settings", &_showFractureSettings))
	{
		this->leaveSpace(1);

		if (ImGui::Button("Launch Algorithm"))
		{
			_fragmentMetadata.clear();
			_fractureText = _scene->fractureGrid(_fragmentMetadata, *_fractureParameters);
		}

		ImGui::SameLine(0, 10);

		if (ImGui::Button("Select Model"))
		{
			_showFileDialog = true;
		}

		this->leaveSpace(2); ImGui::Text("Algorithm Settings"); ImGui::Separator(); this->leaveSpace(2);

		ImGui::SliderInt3("Grid Subdivisions", &_fractureParameters->_gridSubdivisions[0], 1, 756);

		ImGui::PushItemWidth(150.0f);
		{
			ImGui::Combo("Base Algorithm", &_fractureParameters->_fractureAlgorithm, FractureParameters::Fracture_STR, IM_ARRAYSIZE(FractureParameters::Fracture_STR)); ImGui::SameLine(0, 20);
			ImGui::Combo("Distance Function", &_fractureParameters->_distanceFunction, FractureParameters::Distance_STR, IM_ARRAYSIZE(FractureParameters::Distance_STR));
			ImGui::SliderInt("Num. seeds", &_fractureParameters->_numSeeds, 2, 1000); ImGui::SameLine(0, 20);
			ImGui::SliderInt("Num. Extra Seeds", &_fractureParameters->_numExtraSeeds, 0, 1000); ImGui::SameLine(0, 20);
			ImGui::Combo("Seed Random Distribution", &_fractureParameters->_seedingRandom, FractureParameters::Random_STR, IM_ARRAYSIZE(FractureParameters::Random_STR));
			ImGui::Combo("Distance Function (Merge Seeds)", &_fractureParameters->_mergeSeedsDistanceFunction, FractureParameters::Distance_STR, IM_ARRAYSIZE(FractureParameters::Distance_STR)); ImGui::SameLine(0, 20);
			ImGui::Checkbox("Remove Isolated Regions", &_fractureParameters->_removeIsolatedRegions);
			ImGui::SliderInt("Biased Seeds", &_fractureParameters->_biasSeeds, 0, 6); ImGui::SameLine(0, 20);
			ImGui::SliderInt("Spreading of Biased Points", &_fractureParameters->_spreading, 2, 10);
			ImGui::Checkbox("Fill Shape", &_fractureParameters->_fillShape);
			ImGui::SliderInt("Boundary Size", &_fractureParameters->_boundarySize, 1, 10);
			ImGui::SameLine(0, 20); ImGui::Checkbox("Compute Fragment Meshes (Marching cubes)", &_fractureParameters->_computeMCFragments);
		}
		ImGui::PopItemWidth();

		this->leaveSpace(3); ImGui::Text("Execution Settings"); ImGui::Separator(); this->leaveSpace(2);
		ImGui::Checkbox("Use GPU", &_fractureParameters->_launchGPU); ImGui::SameLine(0, 20);
		ImGui::InputInt("Seed", &_fractureParameters->_seed, 1);

		this->leaveSpace(3); ImGui::Text("Erosion Settings"); ImGui::Separator(); this->leaveSpace(2);
		ImGui::PushItemWidth(120.0f);
		{
			ImGui::Checkbox("Erode", &_fractureParameters->_erode);
			ImGui::Combo("Erosion Convolution", &_fractureParameters->_erosionConvolution, FractureParameters::Erosion_STR, IM_ARRAYSIZE(FractureParameters::Erosion_STR));
			ImGui::SameLine(0, 20);  ImGui::SliderInt("Size", &_fractureParameters->_erosionSize, 3, 13);
			ImGui::SameLine(0, 20);  ImGui::SliderInt("Iterations", &_fractureParameters->_erosionIterations, 1, 10);
			ImGui::SliderFloat("Erosion Probability", &_fractureParameters->_erosionProbability, 0.0f, 1.0f);
			ImGui::SameLine(0, 20); ImGui::SliderFloat("Erosion Threshold", &_fractureParameters->_erosionThreshold, 0.0f, 1.0f);
		}
		ImGui::PopItemWidth();

		this->leaveSpace(3); ImGui::Text("Save Result"); ImGui::Separator(); this->leaveSpace(2);
		if (ImGui::Button("Export Fragments"))
		{
			_scene->exportGrid();
		}
	}

	ImGui::End();
}

void GUI::showRenderingSettings()
{
	if (ImGui::Begin("Rendering Settings", &_showRenderingSettings))
	{
		ImGui::ColorEdit3("Background color", &_renderingParams->_backgroundColor[0]);

		this->leaveSpace(3);

		if (ImGui::BeginTabBar("Rendering Tab Bar"))
		{
			if (ImGui::BeginTabItem("General settings"))
			{
				this->leaveSpace(1);

				ImGui::Separator();
				ImGui::Text(ICON_FA_LIGHTBULB "Lighting");

				ImGui::SliderFloat("Scattering", &_renderingParams->_materialScattering, 0.0f, 10.0f);

				this->leaveSpace(2);

				ImGui::Separator();
				ImGui::Text(ICON_FA_TREE "Scenario");

				{
					ImGui::Spacing();

					ImGui::Checkbox("Screen Space Ambient Occlusion", &_renderingParams->_ambientOcclusion);

					const char* visualizationTitles[] = { "Points", "Lines", "Triangles", "All" };
					ImGui::Combo("Visualization", &_renderingParams->_visualizationMode, visualizationTitles, IM_ARRAYSIZE(visualizationTitles));
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Point Cloud"))
			{
				this->leaveSpace(1);

				ImGui::SliderFloat("Point Size", &_renderingParams->_scenePointSize, 0.1f, 50.0f);
				ImGui::ColorEdit3("Point Cloud Color", &_renderingParams->_scenePointCloudColor[0]);
				ImGui::Combo("Rendering type", &_renderingParams->_pointCloudRendering, _renderingParams->PointCloudRendering_STR, IM_ARRAYSIZE(_renderingParams->PointCloudRendering_STR));

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Wireframe"))
			{
				this->leaveSpace(1);

				ImGui::ColorEdit3("Wireframe Color", &_renderingParams->_wireframeColor[0]);
				ImGui::Combo("Rendering type", &_renderingParams->_wireframeRendering, _renderingParams->WireframeRendering_STR, IM_ARRAYSIZE(_renderingParams->WireframeRendering_STR));

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Triangle mesh"))
			{
				this->leaveSpace(1);

				ImGui::Combo("Rendering type", &_renderingParams->_triangleMeshRendering, _renderingParams->TriangleMeshRendering_STR, IM_ARRAYSIZE(_renderingParams->TriangleMeshRendering_STR));
				if (_renderingParams->_triangleMeshRendering == RenderingParameters::VOXELIZATION)
				{
					ImGui::Checkbox("Plane Clipping", &_renderingParams->_planeClipping);
					ImGui::SameLine(0, 20); ImGui::SliderFloat4("Coefficients", &_renderingParams->_planeCoefficients[0], -10.0f, 10.0f);
				}

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}

	ImGui::End();
}

void GUI::showSceneSettings()
{
	if (_modelComponents.empty()) _modelComponents = _renderer->getCurrentScene()->getModelComponents();

	ImGui::SetNextWindowSize(ImVec2(480, 440), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Scene Models", &_showSceneSettings, ImGuiWindowFlags_None))
	{
		this->leaveSpace(4);

		// Left
		static int modelCompSelected = 0;

		ImGui::BeginChild("Model Components", ImVec2(200, 0), true);

		for (int i = 0; i < _modelComponents.size(); ++i)
		{
			if (ImGui::Selectable(_modelComponents[i]->_name.c_str(), modelCompSelected == i))
				modelCompSelected = i;
		}

		ImGui::EndChild();

		ImGui::SameLine();

		// Right
		ImGui::BeginGroup();
		ImGui::BeginChild("Model Component View", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));		// Leave room for 1 line below us

		ImGui::Text(_modelComponents[modelCompSelected]->_name.c_str());
		ImGui::Separator();

		this->leaveSpace(1);

		ImGui::Checkbox("Enabled", &_modelComponents[modelCompSelected]->_enabled);

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	ImGui::End();
}

void GUI::showScreenshotSettings()
{
	if (ImGui::Begin("Screenshot Settings", &_showScreenshotSettings, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::SliderFloat("Size multiplier", &_renderingParams->_screenshotMultiplier, 1.0f, 10.0f);
		ImGui::InputText("Filename", _renderingParams->_screenshotFilenameBuffer, IM_ARRAYSIZE(_renderingParams->_screenshotFilenameBuffer));

		this->leaveSpace(2);

		ImGui::PushID(0);
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1 / 7.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1 / 7.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(1 / 7.0f, 0.8f, 0.8f));

		if (ImGui::Button("Take screenshot"))
		{
			std::string filename = _renderingParams->_screenshotFilenameBuffer;

			if (filename.empty())
			{
				filename = "Screenshot.png";
			}
			else if (filename.find(".png") == std::string::npos)
			{
				filename += ".png";
			}

			Renderer::getInstance()->getScreenshot(filename);
		}

		ImGui::PopStyleColor(3);
		ImGui::PopID();
	}

	ImGui::End();
}

GUI::~GUI()
{
	ImGui::DestroyContext();
}

/// [Public methods]

void GUI::initialize(GLFWwindow* window, const int openGLMinorVersion)
{
	const std::string openGLVersion = "#version 4" + std::to_string(openGLMinorVersion) + "0 core";

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	this->loadImGUIStyle();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(openGLVersion.c_str());
}

void GUI::render()
{
	bool show_demo_window = true;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	//ImGui::ShowDemoWindow(&show_demo_window);

	this->createMenu();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ---------------- IMGUI ------------------

void GUI::loadImGUIStyle()
{
	ImGui::StyleColorsDark();

	this->loadFonts();
}

void GUI::loadFonts()
{
	ImFontConfig cfg;
	ImGuiIO& io = ImGui::GetIO();

	std::copy_n("Lato", 5, cfg.Name);
	io.Fonts->AddFontFromMemoryCompressedBase85TTF(lato_compressed_data_base85, 15.0f, &cfg);

	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	cfg.MergeMode = true;
	cfg.PixelSnapH = true;
	cfg.GlyphMinAdvanceX = 20.0f;
	cfg.GlyphMaxAdvanceX = 20.0f;
	std::copy_n("FontAwesome", 12, cfg.Name);

	io.Fonts->AddFontFromFileTTF("Assets/Fonts/fa-regular-400.ttf", 13.0f, &cfg, icons_ranges);
	io.Fonts->AddFontFromFileTTF("Assets/Fonts/fa-solid-900.ttf", 13.0f, &cfg, icons_ranges);
}