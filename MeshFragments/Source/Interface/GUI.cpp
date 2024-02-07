#include "stdafx.h"
#include "GUI.h"

#include "Graphics/Application/Renderer.h"
#include "imfiledialog/ImGuiFileDialog.h"
#include "Interface/Fonts/font_awesome.hpp"
#include "Interface/Fonts/lato.hpp"
#include "Interface/Fonts/IconsFontAwesome5.h"
#include "Fracturer/Seeder.h"

/// [Protected methods]

GUI::GUI() :
	_showRenderingSettings(false), _showSceneSettings(false), _showScreenshotSettings(false), _showAboutUs(false), 
	_showControls(false), _showFractureSettings(false), _showFileDialog(false), _showFragmentList(false), 
	_modelFilePath(""), _fractureText("")
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
			ImGui::MenuItem(ICON_FA_PALETTE "Rendering", NULL, &_showRenderingSettings);
			ImGui::MenuItem(ICON_FA_IMAGE "Screenshot", NULL, &_showScreenshotSettings);
			ImGui::MenuItem(ICON_FA_HEART_BROKEN "Fracture", NULL, &_showFractureSettings);
			ImGui::MenuItem(ICON_FA_LIST "Fracture List", NULL, &_showFragmentList);
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
	static int selectedFragment = 0;

	if (_fragmentMetadata.empty()) _fragmentMetadata = _scene->getFragmentMetadata();
	if (_fragment.empty()) _fragment = _scene->getFractureMeshes();
	if (_fragmentMetadata.empty()) selectedFragment = 0;

	ImGui::SetNextWindowSize(ImVec2(480, 440), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Fragments", &_showFragmentList, ImGuiWindowFlags_None))
	{
		this->leaveSpace(2);

		// Left
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
		ImGui::BeginChild("Fragment View", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding);		// Leave room for 1 line below us);		// Leave room for 1 line below us

		if (selectedFragment >= 0)
		{
			const std::string name = "Fragment " + std::to_string(selectedFragment + 1);
			ImGui::Text(name.c_str());
			ImGui::Separator();

			this->leaveSpace(1);

			const FragmentationProcedure::FragmentMetadata& metadata = _fragmentMetadata[selectedFragment];
			Model3D* fragment = _fragment[selectedFragment];

			ImGui::Checkbox("Enabled", &fragment->getModelComponent(0)->_enabled);
			this->renderText("Number of voxels: ", std::to_string(metadata._voxels));
			this->renderText("Number of voxels (percentage): ", std::to_string(metadata._percentage));
			this->renderText("Number of occupied voxels in vessel: ", std::to_string(metadata._occupiedVoxels));
			this->renderText("Voxelization size: ", std::to_string(metadata._voxelizationSize.x) + ", " + std::to_string(metadata._voxelizationSize.y) + ", " + std::to_string(metadata._voxelizationSize.z));
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	ImGui::End();
}

void GUI::showFractureSettings()
{
	if (ImGui::Begin("Fracture Settings", &_showFractureSettings))
	{
		if (ImGui::Button("Launch Algorithm"))
		{
			_fragmentMetadata.clear();
			_fragment.clear();
			_fractureText = _scene->fractureGrid(_fragmentMetadata, *_fractureParameters);
		}

		ImGui::SameLine(0, 10);

		if (ImGui::Button("Select Model"))
		{
			_showFileDialog = true;
		}

		ImGui::Checkbox("Use GPU", &_fractureParameters->_launchGPU); ImGui::SameLine(0, 20);
		ImGui::InputInt("Seed", &_fractureParameters->_seed, 1);

		ImGui::PushItemWidth(300.0f);
		if (ImGui::BeginTabBar("Fracture Tab Bar"))
		{
			if (ImGui::BeginTabItem("General settings"))
			{
				ImGui::SliderInt("Grid Subdivisions", &_fractureParameters->_voxelizationSize[0], 1, _fractureParameters->_clampVoxelMetricUnit);

				int maxSeeds = std::pow(2, fracturer::Seeder::VOXEL_ID_POSITION) / 32;

				ImGui::Combo("Distance Function", &_fractureParameters->_distanceFunction, FractureParameters::Distance_STR, IM_ARRAYSIZE(FractureParameters::Distance_STR));
					
				this->leaveSpace(1);
				ImGui::SliderInt("Number of Seeds", &_fractureParameters->_numSeeds, 1, maxSeeds);
				ImGui::SliderInt("Number of Extra Seeds", &_fractureParameters->_numExtraSeeds, 0, std::max(maxSeeds - _fractureParameters->_numSeeds - _fractureParameters->_biasSeeds, 0)); 
				ImGui::Combo("Seed Random Distribution", &_fractureParameters->_seedingRandom, FractureParameters::Random_STR, IM_ARRAYSIZE(FractureParameters::Random_STR));
				ImGui::Combo("Distance Function (Merge Seeds)", &_fractureParameters->_mergeSeedsDistanceFunction, FractureParameters::Distance_STR, IM_ARRAYSIZE(FractureParameters::Distance_STR)); 

				this->leaveSpace(1);
				ImGui::SliderInt("Impacts", &_fractureParameters->_numImpacts, 0, 10);
				ImGui::SliderInt("Biased Seeds", &_fractureParameters->_biasSeeds, 0, maxSeeds - _fractureParameters->_numSeeds); 
				ImGui::SliderInt("Spreading of Biased Points", &_fractureParameters->_biasFocus, 1, 15);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Erosion"))
			{
				ImGui::Checkbox("Erode", &_fractureParameters->_erode);

				this->leaveSpace(1);
				ImGui::Combo("Erosion Convolution", &_fractureParameters->_erosionConvolution, FractureParameters::Erosion_STR, IM_ARRAYSIZE(FractureParameters::Erosion_STR));
				ImGui::SliderInt("Size", &_fractureParameters->_erosionSize, 3, 13);
				ImGui::SliderInt("Iterations", &_fractureParameters->_erosionIterations, 1, 10);

				this->leaveSpace(1);
				ImGui::SliderFloat("Erosion Probability", &_fractureParameters->_erosionProbability, 0.0f, 1.0f);
				ImGui::SliderFloat("Erosion Threshold", &_fractureParameters->_erosionThreshold, 0.0f, 1.0f);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Marching Cubes"))
			{
				ImGui::SliderFloat("Boundary Iterations", &_fractureParameters->_boundaryMCIterations, 0.0f, 0.1f);
				ImGui::SliderFloat("Boundary Weight", &_fractureParameters->_boundaryMCWeight, 0.0f, 1.0f);

				this->leaveSpace(1);

				ImGui::SliderFloat("Non Boundary Iterations", &_fractureParameters->_nonBoundaryMCIterations, 0.0f, 0.1f);
				ImGui::SliderFloat("Non Boundary Weight", &_fractureParameters->_nonBoundaryMCWeight, 0.0f, 1.0f);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Erosion"))
			{
				ImGui::Combo("Grid Extension", &_fractureParameters->_exportMeshExtension, FractureParameters::ExportGrid_STR, IM_ARRAYSIZE(FractureParameters::ExportGrid_STR));
				ImGui::SameLine(0, 20);
				if (ImGui::Button("Export Grid"))
					_scene->exportGrid(*_fractureParameters);

				ImGui::Combo("Mesh Extension", &_fractureParameters->_exportMeshExtension, FractureParameters::ExportMesh_STR, IM_ARRAYSIZE(FractureParameters::ExportMesh_STR));
				ImGui::SameLine(0, 20);
				if (ImGui::Button("Export Fracture Meshes"))
					_scene->exportFragments(*_fractureParameters, FractureParameters::ExportMesh_STR[_fractureParameters->_exportMeshExtension]);

				ImGui::Combo("Point Cloud Extension", &_fractureParameters->_exportPointCloudExtension, FractureParameters::ExportPointCloud_STR, IM_ARRAYSIZE(FractureParameters::ExportPointCloud_STR));
				ImGui::SameLine(0, 20);
				if (ImGui::Button("Export Point Cloud"))
					_scene->exportPointClouds(*_fractureParameters);

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::PopItemWidth();

		ImGui::End();
	}
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

	ImGui::SetNextWindowSize(ImVec2(800, 440), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Models", &_showSceneSettings, ImGuiWindowFlags_None))
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

		ImGui::End();
	}
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

	this->loadStyle();

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

void GUI::loadStyle()
{
	ImGui::StyleColorsDark();
	this->loadFonts();

	ImGuiStyle& style = ImGui::GetStyle();

	style.AntiAliasedFill = true;
	style.AntiAliasedLines = true;
	style.AntiAliasedLinesUseTex = true;

	style.WindowPadding = ImVec2(13, 13);
	style.WindowRounding = 8.0f;
	style.FramePadding = ImVec2(5, 5);
	style.FrameRounding = 5.0f;
	style.ItemSpacing = ImVec2(12, 8);
	style.ItemInnerSpacing = ImVec2(8, 6);
	style.IndentSpacing = 25.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 5.0f;
	style.GrabRounding = 3.0f;
	style.WindowBorderSize = 0.0f;
	style.ChildBorderSize = 0.0f;
	style.PopupBorderSize = 0.0f;
	style.FrameBorderSize = 1.0f;
	style.TabBorderSize = 0.0f;
	style.ChildRounding = 3.0f;
	style.PopupRounding = 2.0f;
	style.LogSliderDeadzone = 4.0f;
	style.TabRounding = 3.0f;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 0.85f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.23f, 0.23f, 0.20f, 0.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.90f, 0.70f, 0.00f, 0.10f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.70f, 0.00f, 0.75f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.78f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.70f, 0.00f, 0.75f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.90f, 0.70f, 0.00f, 0.75f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	colors[ImGuiCol_Separator] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.90f, 0.70f, 0.00f, 0.50f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.90f, 0.70f, 0.00f, 0.75f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
	colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.19f, 0.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
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