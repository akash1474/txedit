#include "pch.h"

#include <filesystem>
#include <windows.h>
#include <pdh.h>

#include "imgui_internal.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "nlohmann/json.hpp"

#include "resources/FontAwesomeRegular.embed"
#include "resources/FontAwesomeSolid.embed"
#include "resources/JetBrainsMonoNLRegular.embed"
#include "resources/JetBrainsMonoNLItalic.embed"

#include "external/FontAwesome6.h"

#include "Timer.h"
#include "CoreSystem.h"
#include "ImageTexture.h"
#include "MultiThreading.h"
#include "Application.h"
#include "utils.h"

#include "ui/FileNavigation.h"
#include "ui/QuickFileSearch.h"
#include "ui/TabsManager.h"
#include "editor/UndoManager.h"
#include "fs/DirectoryFinder.h"


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



void DisplayColorTable(const std::unordered_map<std::string, ImU32>& colorMap) {
    static char filter[128] = ""; // Input buffer for filtering

    ImGui::InputText("Filter", filter, IM_ARRAYSIZE(filter)); // Input text for search

    if (ImGui::BeginTable("ColorTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 150.0f);  // Fixed width for key column
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthFixed, 60.0f); // Set width for color preview

        ImGui::TableHeadersRow();

        for (const auto& [key, color] : colorMap) {
            if (filter[0] != '\0' && key.find(filter) == std::string::npos) {
                continue; // Skip items that don't match the filter
            }

            ImGui::TableNextRow();
            
            // Display key name
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(key.c_str());
            
            // Display actual color preview as a filled rect
            ImGui::TableSetColumnIndex(1);
            ImVec2 pMin = ImGui::GetCursorScreenPos();
            ImVec2 pMax = { pMin.x + 60, pMin.y + 20 }; // Adjust width accordingly
            ImGui::GetWindowDrawList()->AddRectFilled(pMin, pMax, color);
            ImGui::Dummy(ImVec2(60, 20)); // Reserve space for the rectangle
        }
        ImGui::EndTable();
    }
}




void CoreSystem::RenderDebugInfo()
{
	static bool show_demo = true;
	ImGui::ShowDemoWindow(&show_demo);
	Editor* currentEditor=TabsManager::GetCurrentActiveTextEditor();

	ImGui::SetNextWindowRefreshPolicy(ImGuiWindowRefreshFlags_RefreshOnHover);
	if(ImGui::Begin("Project"))
	{

		ShowFPS();
		// ImGui::Text("CPU Usage:%.2f",GetCPUUsage());

		auto& colorMap=ThemeManager::GetCaptureToColorMap();
		DisplayColorTable(colorMap);

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
		std::filesystem::path appDir=GetExecutableDirectoryPath();
		appDir=appDir/"assets/screenshots";

		static ImageTexture img1((appDir/"editor.png").generic_string().c_str());
		static ImageTexture img2((appDir/"multi_cursor.png").generic_string().c_str());
		static ImageTexture img3((appDir/"selection.png").generic_string().c_str());
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
	}
	ImGui::End();
}

#endif


void CoreSystem::RenderAboutPopupWindow(){
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("TxEdit", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("TxEdit: A Lightweight IDE");
        ImGui::Text("Built using C/C++, inspired by SublimeText.");
        ImGui::Text("Purpose: Provide an IDE-like coding experience while being lightweight.");
        ImGui::Text("Features:");
		ImGui::BulletText("Ultra-fast boot time (<350 ms) and quick file loading (<100 ms)");
		ImGui::BulletText("Lightweight IDE with ~10 MB binary size and ~30-150 MB RAM usage");
		ImGui::BulletText("Built-in AI chat assistant powered by Google Gemini, with context-aware responses");
		ImGui::BulletText("Real-time streaming of AI-generated responses inside the editor");
		ImGui::BulletText("Intelligent file search with fuzzy matching and regex support");
		ImGui::BulletText("Real-time syntax highlighting using Tree-sitter AST parsing");
		ImGui::BulletText("Multi-threaded file and image loading using Producer-Consumer pattern");
		ImGui::BulletText("Automatic project-wide file monitoring via Windows API");
		ImGui::BulletText("Integrated terminal using Windows ConPTY API for live shell access");
		ImGui::BulletText("Fast and intelligent auto-completion using a Trie-based engine");
		ImGui::BulletText("Real-time bracket matching using a stack-based algorithm");
		ImGui::BulletText("Multi-cursor editing with support for triple-click and block selection");
		ImGui::BulletText("Dynamic UI with animated status bar, notifications, and tab management");
		ImGui::BulletText("Modular architecture for clean separation of components and easy maintainability");

        // Display a warning for the current development state
        // ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Warning: Still under development, may contain bugs!");

        ImGui::Separator();

        // More information
        ImGui::Text("License: MIT License");
        ImGui::Text("Development Branch: 'dev'");

        // Close the popup
        if (ImGui::Button("Close"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();  // End popup
    }
}




void CoreSystem::Render()
{

	static const ImGuiIO& io = ImGui::GetIO();
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
	static ImGuiWindowFlags window_flags = ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar;


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
			std::filesystem::path layoutConfigPath=GetCurrentWorkingDirectoryPath()/".cache/layout.ini";
			if(!std::filesystem::exists(layoutConfigPath))
			{
				ImGui::DockBuilderRemoveNode(Get().mDockSpaceId); // clear any previous layout
				ImGui::DockBuilderAddNode(Get().mDockSpaceId, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
				ImGui::DockBuilderSetNodeSize(Get().mDockSpaceId, size);

				Get().mLeftDockSpaceId = ImGui::DockBuilderSplitNode(Get().mDockSpaceId, ImGuiDir_Left, 0.3f, nullptr, &Get().mDockSpaceId);
				Get().mRightDockSpaceId = ImGui::DockBuilderSplitNode(Get().mDockSpaceId, ImGuiDir_Right, 0.3f, nullptr, &Get().mDockSpaceId);
				auto dock_id_left_bottom = ImGui::DockBuilderSplitNode(Get().mLeftDockSpaceId, ImGuiDir_Down, 0.3f, nullptr, &Get().mLeftDockSpaceId);
				ImGui::DockBuilderDockWindow("Project Directory", Get().mLeftDockSpaceId);
				ImGui::DockBuilderDockWindow("Chat Window", Get().mLeftDockSpaceId);
				ImGui::DockBuilderDockWindow("Directory Finder", Get().mRightDockSpaceId);
				ImGui::DockBuilderDockWindow("Terminal", dock_id_left_bottom);
#ifdef GL_DEBUG
				ImGui::DockBuilderDockWindow("Dear ImGui Demo", Get().mLeftDockSpaceId);
				ImGui::DockBuilderDockWindow("Project", Get().mLeftDockSpaceId);
#endif
				ImGui::DockBuilderFinish(Get().mDockSpaceId);
			}
			// ImGui::LoadIniSettingsFromDisk("layout.ini");
			TabsManager::SetNewTabsDockSpaceId(Get().mDockSpaceId);
			DirectoryFinder::SetDockspaceId(Get().mRightDockSpaceId);
			LoadDockingLayoutCache();
		}
	}


	RenderMenuBar();
	if(Get().mShowAboutWindow)
	{
        ImGui::OpenPopup("TxEdit");
        Get().mShowAboutWindow=false;
	}

	RenderAboutPopupWindow();

	ImGui::End();


#ifdef GL_DEBUG
	RenderDebugInfo();
#endif

	if (Get().mShowFileNavigation)
		FileNavigation::Render();


	if(Get().mShowChatWindow)
		Get().mChatWindow.Render();

	if(Get().mShowTerminal)
		Get().mTerminal.Render();

	StatusBarManager::Render(size, viewport);
	DirectoryFinder::Render();
	TabsManager::Render();

	QuickFileSearch::Render();

	MultiThreading::ImageLoader::LoadImages();

	if(ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_O))
	{
		const std::string path=SelectFile();
		if(!path.empty())
			TabsManager::OpenTabWithFilePath(path);
	}	

	if(ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyDown(ImGuiKey_ModShift) && ImGui::IsKeyPressed(ImGuiKey_F))
	{
		const std::string path = SelectFolder();
		if (!path.empty())
		{
			const std::string folderPath=std::filesystem::path(path).generic_string();
			FileNavigation::AddFolder(folderPath);
		}
	}

	if(ImGui::IsKeyDown(ImGuiKey_ModCtrl)&& !ImGui::IsKeyDown(ImGuiKey_ModShift) && ImGui::IsKeyPressed(ImGuiKey_F))
		StatusBarManager::ShowFileSearchPanel();

	if(ImGui::IsKeyDown(ImGuiKey_ModShift) && ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_S))
		DirectoryFinder::Show();


	QuickFileSearch::EventListener();

}


void CoreSystem::RenderMenuBar(){
    if (ImGui::BeginMenuBar())
    {
        // File Menu
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New File","Ctrl+N"))
				TabsManager::OpenNewEmptyFile();
            if (ImGui::MenuItem("Open File...", "Ctrl+O")) {
				const std::string path=SelectFile();
				if(!path.empty())
					TabsManager::OpenTabWithFilePath(path);
            }
			if (ImGui::MenuItem("Open Folder","Ctrl+Shift+F")) {
				const std::string path = SelectFolder();
				if (!path.empty())
				{
					const std::string folderPath=std::filesystem::path(path).generic_string();
					FileNavigation::AddFolder(folderPath);
				}
			}
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
            	TabsManager::SaveFile();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
            	Editor* editor=TabsManager::GetCurrentActiveTextEditor();
            	if(editor){
	            	std::string content=editor->GetFullText();
	            	SaveFileAs(content);
            	}
            }
            // if (ImGui::MenuItem("Close", "Ctrl+W")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
            	Application::Close();
            }
            ImGui::EndMenu();
        }

        // Edit Menu
        if (ImGui::BeginMenu("Edit"))
        {
            Editor* editor=TabsManager::GetCurrentActiveTextEditor();
            if (ImGui::MenuItem("Undo", "Ctrl+Z") && editor) {
            	editor->GetUndoMananger()->Undo(1,editor);
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y") && editor) {
            	editor->GetUndoMananger()->Redo(1,editor);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X") && editor) {
            	editor->Cut();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            	editor->Copy();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            	editor->Paste();
            }
            if (ImGui::MenuItem("Select All", "Ctrl+A")) {
            	editor->SelectAll();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Toggle Comment", "Ctrl+/")) {
            	editor->ToggleComments();
            }
            if (ImGui::BeginMenu("Line")) {
                if (ImGui::MenuItem("Indent", "Tab")) {
                	editor->InsertTab(false);
                }
                if (ImGui::MenuItem("Unindent", "Shift+Tab")) {
                	editor->InsertTab(true);
                }
                if (ImGui::MenuItem("Swap Line Up", "Ctrl+Shift+Up")) {
                	editor->SwapLines(true);
                }
                if (ImGui::MenuItem("Swap Line Down", "CtrlShift+Down"))
                	editor->SwapLines(false);

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        // View Menu
        if (ImGui::BeginMenu("Find"))
        {
            if(ImGui::MenuItem("Find...","Ctrl+F")){
            	StatusBarManager::ShowFileSearchPanel();
            }
            if(ImGui::MenuItem("Fuzzy Find File...","Ctrl+P")){
            	QuickFileSearch::ShowQuickSearch();
            }
            // if(ImGui::MenuItem("Find Next")){}
            // if(ImGui::MenuItem("Find Previous")){}
            if(ImGui::MenuItem("Find in Folder..")){
            	DirectoryFinder::Show();
            }
            // if(ImGui::MenuItem("Show Syntactic Error")){}
            ImGui::EndMenu();
        }

        // View Menu
        if (ImGui::BeginMenu("View"))
        {
			ImGui::MenuItem("Chat Window",0,&Get().mShowChatWindow);
			ImGui::MenuItem("File Explorer",0,&Get().mShowFileNavigation);
			ImGui::MenuItem("Terminal",0,&Get().mShowTerminal);
			if(ImGui::MenuItem("Show Syntactic Error",0,&Get().mShowSyntacticError)){
				if(Get().mShowSyntacticError)
					TabsManager::GetCurrentActiveTextEditor()->ReparseEntireTree();
			}
            ImGui::Separator();
            // if (ImGui::MenuItem("Show Line Numbers", "Ctrl+Shift+L")) {}
            if (ImGui::MenuItem("Toggle Fullscreen", "F11")) {
            	
            }
            ImGui::EndMenu();
        }

        // Tools Menu
        // if (ImGui::BeginMenu("Tools"))
        // {
        //     if (ImGui::MenuItem("Build Project", "Ctrl+B")) {}
        //     if (ImGui::MenuItem("Run", "Ctrl+R")) {}
        //     ImGui::Separator();
        //     if (ImGui::MenuItem("Open Terminal", "Ctrl+T")) {}
        //     ImGui::EndMenu();
        // }

        // Help Menu
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Documentation", "F1")) {}
            if (ImGui::MenuItem("About")) {
            	Get().mShowAboutWindow=true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
	}
}

void CoreSystem::CacheDockingLayout(){
	OpenGL::ScopedTimer timer("CoreSystem::CacheDockingLayout");
	auto& folders=FileNavigation::GetFolders();
	nlohmann::json j;
	j["folders"] = folders;

	j["view"]={
		{"terminal",Get().mShowTerminal},
		{"chatWindow",Get().mShowChatWindow},
		{"syntaxError",Get().mShowSyntacticError},
		{"fileExplorer",Get().mShowFileNavigation}
	};

	const std::vector<FileTab>& tabs=TabsManager::GetAllTabs();
	for(auto& tab:tabs)
	{
		j["tabs"].push_back({{"id", tab.id}, {"filepath", tab.filepath}});
	}


	std::ofstream file(GetCurrentWorkingDirectoryPath()/".cache/config.json");
	file << j.dump(4); // Pretty print with indentation
	file.close();
}

void CoreSystem::LoadDockingLayoutCache() {
	OpenGL::ScopedTimer timer("CoreSystem::LoadDockingLayoutCache");
    std::ifstream file(GetCurrentWorkingDirectoryPath()/".cache/config.json");
    if (!file.is_open()) {
        GL_WARN("No cached docking layout found.");
        return;
    }

    nlohmann::json j;
    file >> j;
    file.close();

    if(j.contains("view") && j["view"].is_object()){
        const auto& view = j["view"];

        Get().mShowChatWindow = view.value("chatWindow", Get().mShowChatWindow);
        Get().mShowSyntacticError = view.value("syntaxError", Get().mShowSyntacticError);
        Get().mShowTerminal = view.value("terminal", Get().mShowTerminal);
        Get().mShowFileNavigation = view.value("fileExplorer", Get().mShowFileNavigation);
    }

    // Retrieve folders
    if (j.contains("folders")) {
        auto folders = j["folders"].get<std::vector<std::string>>();
        for (const auto& folder : folders) {
            FileNavigation::AddFolder(folder);
        }
    }

    // Retrieve tabs
    if (j.contains("tabs")) {
        auto tabs = j["tabs"];
        for (const auto& tab : tabs) {
            std::string id = tab["id"].get<std::string>();
            std::string filepath = tab["filepath"].get<std::string>();

            TabsManager::InitializeTabFromCache(id,filepath); // Adjust if OpenFile requires only filepath or also accepts id
        }
    }
}



bool CoreSystem::Init() {
	return true; 
}

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
	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLRegular, IM_ARRAYSIZE(JetBrainsMonoNLRegular), font_size, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)FontAwesomeSolid, IM_ARRAYSIZE(FontAwesomeSolid), (font_size + 4.0f) * 2.0f / 3.0f, &icon_config,icons_ranges);

	// io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuii.ttf",font_size+3,&font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLItalic, IM_ARRAYSIZE(JetBrainsMonoNLItalic), font_size + 2.0f, &font_config);

	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLRegular, IM_ARRAYSIZE(JetBrainsMonoNLRegular), font_size, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)FontAwesomeRegular, IM_ARRAYSIZE(FontAwesomeRegular), (font_size + 4.0f) * 2.0f / 3.0f,&icon_config, icons_ranges);
	
	std::filesystem::path appDir=GetExecutableDirectoryPath();
	std::string fontDir=(appDir/"assets/fonts").generic_string();
    io.Fonts->AddFontFromFileTTF( (fontDir + "/AROneSans-Regular.ttf").c_str(), 24 ,&font_config);
    io.Fonts->AddFontFromFileTTF( (fontDir + "/AROneSans-Bold.ttf").c_str(), 28 ,&font_config);
    io.Fonts->AddFontFromFileTTF( (fontDir + "/AROneSans-Bold.ttf").c_str(), 36 ,&font_config);
    io.Fonts->AddFontFromFileTTF( (fontDir + "/AROneSans-Bold.ttf").c_str(), 32 ,&font_config);
    io.Fonts->AddFontFromFileTTF( (fontDir + "/AROneSans-Medium.ttf").c_str(), 24 ,&font_config);

	io.Fonts->AddFontFromMemoryTTF((void*)JetBrainsMonoNLRegular, IM_ARRAYSIZE(JetBrainsMonoNLRegular), font_size, &font_config);
}
