#include "pch.h"
#include "GLFW/glfw3.h"
#include "imgui.h"

#include "resources/FontAwesomeRegular.embed"
#include "resources/FontAwesomeSolid.embed"
#include "resources/MonoLisaRegular.embed"
#include "resources/JetBrainsMonoNLRegular.embed"
#include "resources/JetBrainsMonoNLItalic.embed"

#include "CoreSystem.h"
#include "ImageTexture.h"
#include "MultiThreading.h"
#include "Terminal.h"
#include "FileNavigation.h"
#include "TabsManager.h"

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
	ImGui::Text("Time:%f", glfwGetTime());
	ImGui::Text("nCursor:%d", (int)currentEditor->GetEditorState()->mCursors.size());

	ImWchar buff[100];
	int count = ImTextStrFromUtf8(buff, 100, utf8, nullptr, 0);

	static int LineSpacing = 15.0f;
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
	if (ImGui::InputInt("##ScrollToLine", &v, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
		currentEditor->ScrollToLineNumber(v);

	if (ImGui::Button("Select File"))
		SelectFile();

	if (ImGui::Button("Select Files")) {
		auto files = SelectFiles();
		for (auto& file : files) std::wcout << file << std::endl;
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
	// ImageTexture::AsyncImage(imgs[1],ImVec2(362, 256));
	// ImageTexture::AsyncImage(imgs[2],ImVec2(362, 256));
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
	ImVec2 size(viewport->WorkSize.x, StatusBarManager::IsInputPanelOpen()
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
			// ImGui::DockBuilderDockWindow("#editor_container", Get().mRightDockSpaceId);
			ImGui::DockBuilderDockWindow("Terminal", dock_id_left_bottom);
#ifdef GL_DEBUG
			ImGui::DockBuilderDockWindow("Dear ImGui Demo", Get().mLeftDockSpaceId);
			ImGui::DockBuilderDockWindow("Project", Get().mLeftDockSpaceId);
#endif
			ImGui::DockBuilderFinish(Get().mDockSpaceId);

			TabsManager::SetNewTabsDockSpaceId(Get().mDockSpaceId);
		}
	}


	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if(ImGui::MenuItem("New File"))
				TabsManager::OpenNewEmptyFile();

			if (ImGui::MenuItem("Open File"))
				TabsManager::OpenFile(SelectFile().c_str());

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
	//RenderDebugInfo();
#endif

	if (FileNavigation::IsOpen())
		FileNavigation::Render();
	Get().mTerminal.Render();
	StatusBarManager::Render(size, viewport);
	TabsManager::Render();
	MultiThreading::ImageLoader::LoadImages();
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
	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLRegular, IM_ARRAYSIZE(JetBrainsMonoNLRegular), font_size + 2, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)FontAwesomeSolid, IM_ARRAYSIZE(FontAwesomeSolid), (font_size + 4.0f) * 2.0f / 3.0f, &icon_config,
	                               icons_ranges);

	// io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuii.ttf",font_size+3,&font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLItalic, IM_ARRAYSIZE(JetBrainsMonoNLItalic), font_size + 2, &font_config);

	io.Fonts->AddFontFromMemoryTTF((void*)MonoLisaRegular, IM_ARRAYSIZE(MonoLisaRegular), font_size - 4.0f, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)FontAwesomeRegular, IM_ARRAYSIZE(FontAwesomeRegular), (font_size + 4.0f) * 2.0f / 3.0f,
	                               &icon_config, icons_ranges);
}
