#include "FontAwesome6.h"
#include "QuickFileSearch.h"
#include "Timer.h"
#include "pch.h"
#include "imgui_internal.h"
#include "GLFW/glfw3.h"
#include "imgui.h"

#include "resources/FontAwesomeRegular.embed"
#include "resources/FontAwesomeSolid.embed"
#include "resources/JetBrainsMonoNLRegular.embed"
#include "resources/JetBrainsMonoNLItalic.embed"

#include "CoreSystem.h"
#include "ImageTexture.h"
#include "MultiThreading.h"
#include "FileNavigation.h"
#include "TabsManager.h"
#include "DirectoryFinder.h"
#include "QuickFileSearch.h"
#include <filesystem>

#ifdef min
	#undef min
#endif
#ifdef GL_DEBUG

void ShowFPS()
{
	static float previousTime = 0.0f;
	static int frameCount = 0;
	static float fps = 0.0f;

	// Get the current time
	float currentTime = static_cast<float>(glfwGetTime());
	frameCount++;

	// Calculate FPS every 0.5 seconds
	if (currentTime - previousTime >= 0.5f) {
		fps = frameCount / (currentTime - previousTime);
		previousTime = currentTime;
		frameCount = 0;
	}

	ImGui::Text("FPS: %.1f", fps);
}

float EaseOutQuadraticFn(float t) { return 1.0f - pow(1.0f - t, 4);}

void CoreSystem::RenderDebugInfo()
{
	static bool show_demo = true;
	ImGui::ShowDemoWindow(&show_demo);
	Editor* currentEditor=TabsManager::GetCurrentActiveTextEditor();

	ImGui::SetNextWindowRefreshPolicy(ImGuiWindowRefreshFlags_RefreshOnFocus);
	ImGui::Begin("Project");
	ShowFPS();
	const char* utf8 = "Mastering » Ñandú.txt";
	ImGui::Text("%s", utf8);
	ImGui::Text("Length:%d", ImTextCountCharsFromUtf8(utf8, 0));
	ImGui::Text("GLFW::Time:%f", glfwGetTime());
	ImGui::Text("ImGui::Time:%f", (float)ImGui::GetTime());
	static float add=0.5f,scale=0.5f;
	static int speed=2;
	ImGui::SliderInt("Speed", &speed, 1, 10);
	ImGui::SliderFloat("Added", &add, 0.0f, 1.0f);
	ImGui::SliderFloat("Scale", &scale, 0.0f, 1.0f);
	float alpha = add + scale * sin((float)ImGui::GetTime() * speed);
	float value=EaseOutQuadraticFn(alpha);
	ImGui::Text("Value:%.2f",alpha);
	ImGui::SliderFloat("##slider", &alpha, -1.0f, 1.0f);
	ImGui::SliderFloat("##slider", &value, -1.0f, 1.0f);



	if(currentEditor)
	{
		static char buff[1024]="";
		static bool isFirst=true;

		ImGui::Text("nCursor:%d", (int)currentEditor->GetEditorState()->mCursors.size());
		static int LineSpacing = 8.0f;
		if (ImGui::SliderInt("LineSpacing", &LineSpacing, 0, 20)) {
			currentEditor->SetLineSpacing(LineSpacing);
		}


		ImGui::Spacing();
		ImGui::Text("PositionY:%.2f", ImGui::GetMousePos().y);
		ImGui::Spacing();
		ImGui::Text("mCursorPosition: X:%d  Y:%d", currentEditor->GetCurrentCursor().mCursorPosition.mColumn,
		            currentEditor->GetCurrentCursor().mCursorPosition.mLine);
		ImGui::Text("mSelectionStart: X:%d  Y:%d", currentEditor->GetCurrentCursor().mSelectionStart.mColumn,
		            currentEditor->GetCurrentCursor().mSelectionStart.mLine);
		ImGui::Text("mSelectionEnd:   X:%d  Y:%d", currentEditor->GetCurrentCursor().mSelectionEnd.mColumn,
		            currentEditor->GetCurrentCursor().mSelectionEnd.mLine);
		ImGui::Text("mUndoManagerTop:   X:%d  Y:%d", currentEditor->GetCurrentCursor().mSelectionEnd.mColumn,
		            currentEditor->GetCurrentCursor().mSelectionEnd.mLine);

		ImGui::Spacing();
		currentEditor->GetUndoMananger()->DisplayUndoStack();

		ImGui::Spacing();
		ImGui::Spacing();
		static std::string mode;
		ImColor color;
		switch (currentEditor->GetSelectionMode()) {
			case 0:
				mode = "Normal";
				color = ImColor(50, 206, 187, 255);
				break;
			case 1:
				mode = "Word";
				color = ImColor(233, 196, 106, 255);
				break;
			case 2:
				mode = "Line";
				color = ImColor(231, 111, 81, 255);
				break;
		}
		ImGui::Text("SelectionMode: ");
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, color.Value);
		ImGui::Text("%s", mode.c_str());
		ImGui::PopStyleColor();


		ImGui::Spacing();
		ImGui::Spacing();
		static int v{1};
		ImGui::Text("Goto Line: ");
		ImGui::SameLine();
		if (ImGui::InputInt("##ScrollToLine", &v, 1, 100))
			currentEditor->ScrollToLineNumber(v);

	}
	#ifdef GL_DEBUG

	static ImageTexture img1("./assets/screenshots/editor.png");
	static ImageTexture img2("./assets/screenshots/multi_cursor.png");
	static ImageTexture img3("./assets/screenshots/selection.png");
	static bool pushed = false;
	if (!pushed) {
		MultiThreading::ImageLoader::PushImageToQueue(&img1);
		MultiThreading::ImageLoader::PushImageToQueue(&img2);
		MultiThreading::ImageLoader::PushImageToQueue(&img3);
		pushed = true;
	}
	
	ImageTexture::AsyncImage(&img1, ImVec2(362, 256));
	ImageTexture::AsyncImage(&img2, ImVec2(362, 256));
	ImageTexture::AsyncImage(&img3, ImVec2(362, 256));
	#endif


	ImGui::End();
}

#endif






void CoreSystem::Render()
{

	static const ImGuiIO& io = ImGui::GetIO();
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
	static ImGuiWindowFlags window_flags = ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar ;


	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImVec2 size(viewport->WorkSize.x, StatusBarManager::IsAnyPanelOpen()
	                                      ? viewport->WorkSize.y - StatusBarManager::StatusBarSize - StatusBarManager::PanelSize
	                                      : viewport->WorkSize.y - StatusBarManager::StatusBarSize);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Container", nullptr, window_flags | ImGuiWindowFlags_NoResize);
	ImGui::PopStyleVar(3);


	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
		Get().mDockSpaceId = ImGui::GetID("DDockSpace");
		ImGui::DockSpace(Get().mDockSpaceId, ImVec2(0.0f, 0.0f), dockspace_flags);

		static bool setupRequired = true;
		if (setupRequired) {
			setupRequired = false;
			ImGui::DockBuilderRemoveNode(Get().mDockSpaceId); // clear any previous layout
			ImGui::DockBuilderAddNode(Get().mDockSpaceId, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(Get().mDockSpaceId, size);

			Get().mLeftDockSpaceId = ImGui::DockBuilderSplitNode(Get().mDockSpaceId, ImGuiDir_Left, 0.3f, nullptr, &Get().mDockSpaceId);
			Get().mRightDockSpaceId = ImGui::DockBuilderSplitNode(Get().mDockSpaceId, ImGuiDir_Right, 0.3f, nullptr, &Get().mDockSpaceId);
			auto dock_id_left_bottom = ImGui::DockBuilderSplitNode(Get().mLeftDockSpaceId, ImGuiDir_Down, 0.3f, nullptr, &Get().mLeftDockSpaceId);
			ImGui::DockBuilderDockWindow("Project Directory", Get().mLeftDockSpaceId);
			ImGui::DockBuilderDockWindow("Directory Finder", Get().mRightDockSpaceId);
			ImGui::DockBuilderDockWindow("Terminal", dock_id_left_bottom);
#ifdef GL_DEBUG
			ImGui::DockBuilderDockWindow("Dear ImGui Demo", Get().mLeftDockSpaceId);
			ImGui::DockBuilderDockWindow("Project", Get().mLeftDockSpaceId);
#endif
			ImGui::DockBuilderFinish(Get().mDockSpaceId);

			TabsManager::SetNewTabsDockSpaceId(Get().mDockSpaceId);
			DirectoryFinder::SetDockspaceId(Get().mRightDockSpaceId);
		}
	}


	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if(ImGui::MenuItem("New File"))
				TabsManager::OpenNewEmptyFile();

			if (ImGui::MenuItem("Open File"))
			{
				std::string path=SelectFile();
				if(!path.empty())
					TabsManager::OpenFile(path);
			}

			if (ImGui::MenuItem("Open Folder")) {
				std::string path = SelectFolder();
				if (!path.empty())
					FileNavigation::AddFolder(path.c_str());
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			ImGui::MenuItem("Cut");
			ImGui::MenuItem("Copy");
			ImGui::MenuItem("Paste");
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::End();


#ifdef GL_DEBUG
	RenderDebugInfo();
#endif

	if (FileNavigation::IsOpen())
		FileNavigation::Render();


	Get().mTerminal.Render();
	StatusBarManager::Render(size, viewport);
	DirectoryFinder::Render();
	TabsManager::Render();

	QuickFileSearch::Render();

	MultiThreading::ImageLoader::LoadImages();

	if(ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_F))
		StatusBarManager::ShowFileSearchPanel();

	if(ImGui::IsKeyDown(ImGuiKey_ModShift) && ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_S))
	{
		Editor* currEditor=TabsManager::GetCurrentActiveTextEditor();
		if(currEditor)
		{
			std::string parentDir=std::filesystem::path(currEditor->GetCurrentFilePath()).parent_path().generic_string();
			if(std::filesystem::exists(parentDir))
			{
				DirectoryFinder::Setup(parentDir,false);
			}
		}
		else
		{
			auto& folders=FileNavigation::GetFolders();
			if(!folders.empty())
			{
				DirectoryFinder::Setup(folders[0],false);
			}else{
				DirectoryFinder::Setup("",false);
			}
		}
	}


	QuickFileSearch::EventListener();
}


bool CoreSystem::Init() { return true; }

bool CoreSystem::InitImGui() { return true; }

float GetFontSize()
{
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	float baseSize = 12.0f;
	float scaleFactor = 0.007f;
	float fontSize = baseSize + std::min(screenWidth, screenHeight) * scaleFactor;
	return fontSize;
}


void CoreSystem::InitFonts()
{
	GL_INFO("Initializing Fonts");
	const ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	ImFontConfig icon_config;
	icon_config.MergeMode = true;
	icon_config.PixelSnapH = true;
	icon_config.FontDataOwnedByAtlas = false;

	ImFontConfig font_config;
	font_config.FontDataOwnedByAtlas = false;
	const float font_size = GetFontSize();
	// io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf",font_size+3,&font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLRegular, IM_ARRAYSIZE(JetBrainsMonoNLRegular), font_size+2.0f, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)FontAwesomeSolid, IM_ARRAYSIZE(FontAwesomeSolid), (font_size + 4.0f) * 2.0f / 3.0f, &icon_config,
	                               icons_ranges);

	// io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuii.ttf",font_size+3,&font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLItalic, IM_ARRAYSIZE(JetBrainsMonoNLItalic), font_size + 2.0f, &font_config);

	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLRegular, IM_ARRAYSIZE(JetBrainsMonoNLRegular), font_size+2.0f, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)FontAwesomeRegular, IM_ARRAYSIZE(FontAwesomeRegular), (font_size + 4.0f) * 2.0f / 3.0f,
	                               &icon_config, icons_ranges);
}
