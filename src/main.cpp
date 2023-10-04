#include "pch.h"
#include "Log.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#define STB_IMAGE_IMPLEMENTATION
#include "GLFW/glfw3.h"
#include <GLFW/glfw3native.h>
#include "imgui.h"
#include <filesystem>
#include <winuser.h>
#include "TextEditor.h"
#include "stb_img.h"
#include "Image.h"
#include "windows_48.embed"
#include "windows_img.embed"

#define WIDTH 900
#define HEIGHT 600

int width{0};
int height{0};
GLFWwindow* window;

std::string file_data{0};
Editor editor;
bool isTitleBarHovered=false;


size_t size{0};
bool showCustumTitleBar=true;

bool IsMaximized(GLFWwindow* window){
	return (bool)glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
}



void UI_DrawTitlebar(float& outTitlebarHeight)
{
	static bool areIconsLoaded=false;
	static OpenGL::Image logo;
	static OpenGL::Image win_close;
	static OpenGL::Image win_max;
	static OpenGL::Image win_min;
	static OpenGL::Image win_res;

	if(!areIconsLoaded){
		logo.LoadFromBuffer(window_logo,IM_ARRAYSIZE(window_logo));
		win_close.LoadFromBuffer(window_close,IM_ARRAYSIZE(window_close));
		win_min.LoadFromBuffer(window_minimize,IM_ARRAYSIZE(window_minimize));
		win_max.LoadFromBuffer(window_maximize,IM_ARRAYSIZE(window_maximize));
		win_res.LoadFromBuffer(window_restore,IM_ARRAYSIZE(window_restore));
		areIconsLoaded=true;
	}


	const float titlebarHeight = 30.0f;
	const bool isMaximized = IsMaximized(window);
	float titlebarVerticalOffset = isMaximized ? -6.0f : 0.0f;
	const ImVec2 windowPadding = ImGui::GetCurrentWindow()->WindowPadding;

	ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));
	const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
	const ImVec2 titlebarMax = { ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f,
								 ImGui::GetCursorScreenPos().y + titlebarHeight };
	auto* bgDrawList = ImGui::GetBackgroundDrawList();
	auto* fgDrawList = ImGui::GetForegroundDrawList();
	bgDrawList->AddRectFilled(titlebarMin, titlebarMax, IM_COL32(21, 21, 21, 255));
	// DEBUG TITLEBAR BOUNDS
	fgDrawList->AddRect(titlebarMin, titlebarMax, IM_COL32(222, 43, 43, 255));

	// Logo
	{
		const int logoWidth = 24.0f;// m_LogoTex->GetWidth();
		const int logoHeight = 24.0f;// m_LogoTex->GetHeight();
		const ImVec2 logoOffset(10.0f+windowPadding.x, 2.0f);
		const ImVec2 logoRectStart = { ImGui::GetItemRectMin().x + logoOffset.x, ImGui::GetItemRectMin().y + logoOffset.y };
		const ImVec2 logoRectMax = { logoRectStart.x + logoWidth, logoRectStart.y + logoHeight };
		fgDrawList->AddImage((void*)(intptr_t)logo.GetTexture(), logoRectStart, logoRectMax);
	}
	
	ImGui::SetNextWindowPos({0,0});
	ImGui::BeginChild("##TitleBar",{ImGui::GetWindowWidth(),titlebarHeight},true,ImGuiWindowFlags_NoDecoration);
	static float moveOffsetX;
	static float moveOffsetY;
	const float w = ImGui::GetContentRegionAvail().x;
	const float buttonsAreaWidth = 94;

	// Title bar drag area
	// On Windows we hook into the GLFW win32 window internals
	ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset)); // Reset cursor pos
	// DEBUG DRAG BOUNDS
	fgDrawList->AddRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + w - buttonsAreaWidth, ImGui::GetCursorScreenPos().y + titlebarHeight), IM_COL32(222, 43, 43, 255));
	ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - buttonsAreaWidth, titlebarHeight));

	isTitleBarHovered = ImGui::IsItemHovered();

	if (isMaximized)
	{
		float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y;
		if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
			isTitleBarHovered = true; // Account for the top-most pixels which don't register
	}

	// Draw Menubar
	// if (m_MenubarCallback)
	// {
	// 	ImGui::SuspendLayout();
	// 	{
	// 		ImGui::SetItemAllowOverlap();
	// 		const float logoHorizontalOffset = 16.0f * 2.0f + 48.0f + windowPadding.x;
	// 		ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 6.0f + titlebarVerticalOffset));
	// 		UI_DrawMenubar();

	// 		if (ImGui::IsItemHovered())
	// 			m_TitleBarHovered = false;
	// 	}

	// 	ImGui::ResumeLayout();
	// }

	{
		// Centered Window title
		ImVec2 currentCursorPos = ImGui::GetCursorPos();
		ImVec2 textSize = ImGui::CalcTextSize("Window title");
		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - textSize.x * 0.5f, 2.0f + windowPadding.y + 6.0f));
		ImGui::Text("%s", "Window Title"); // Draw title
		ImGui::SetCursorPos(currentCursorPos);
	}

	// Window buttons
	// const ImU32 buttonColN = IM_COL32(150, 150, 150, 255);
	// const ImU32 buttonColH = IM_COL32(220, 220, 220, 255);
	// const ImU32 buttonColP = IM_COL32(128, 128, 128, 255);
	// const float buttonWidth = 14.0f;
	// const float buttonHeight = 14.0f;

	// // Minimize Button

	// ImGui::Spring();
	// ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
	// {
	// 	const int iconWidth = win_min.GetWidth();
	// 	const int iconHeight = win_min.GetHeight();
	// 	const float padY = (buttonHeight - (float)iconHeight) / 2.0f;
	// 	if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight)))
	// 	{
	// 			glfwIconifyWindow(window);
	// 	}

	// 	// UI::DrawButtonImage((void*)(intptr_t)win_min.GetTexture(), buttonColN, buttonColH, buttonColP, UI::RectExpanded(UI::GetItemRect(), 0.0f, -padY));
	// 	ImGui::ImageButton((void*)(intptr_t)win_min.GetTexture(),{4.0f,14.0f});
	// }


	// // Maximize Button
	// ImGui::Spring(-1.0f, 17.0f);
	// ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
	// {
	// 	const int iconWidth = win_max.GetWidth();
	// 	const int iconHeight = win_max.GetHeight();

	// 	const bool isMaximized = IsMaximized(window);

	// 	if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
	// 	{
	// 			if (isMaximized)
	// 				glfwRestoreWindow(window);
	// 			else
	// 				glfwMaximizeWindow(window);
	// 	}

	// 	// UI::DrawButtonImage(isMaximized ? m_IconRestore : m_IconMaximize, buttonColN, buttonColH, buttonColP);
	// 	ImGui::ImageButton(isMaximized ?(void*)(intptr_t)win_res.GetTexture() : (void*)(intptr_t)win_max.GetTexture(),{4.0f,14.0f});
	// }

	// Close Button
	// ImGui::Spring(-1.0f, 15.0f);
	// ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
	// {
	// 	const int iconWidth = win_close.GetWidth();
	// 	const int iconHeight = win_close.GetHeight();
	// 	if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
	// 		glfwDestroyWindow(window);
	// 		// Application::Get().Close();

	// 	// UI::DrawButtonImage(m_IconClose, UI::Colors::Theme::text, UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::text, 1.4f), buttonColP);
	// 	ImGui::ImageButton((void*)(intptr_t)win_close.GetTexture(),{4.0f,14.0f});
	// }

	// ImGui::Spring(-1.0f, 18.0f);
	ImGui::EndChild();

	outTitlebarHeight = titlebarHeight;
}



// Exposed to be used for window with disabled decorations
// This border is going to be drawn even if window border size is set to 0.0f
void RenderWindowOuterBorders(ImGuiWindow* window)
{
	struct ImGuiResizeBorderDef
	{
		ImVec2 InnerDir;
		ImVec2 SegmentN1, SegmentN2;
		float  OuterAngle;
	};

	static const ImGuiResizeBorderDef resize_border_def[4] =
	{
		{ ImVec2(+1, 0), ImVec2(0, 1), ImVec2(0, 0), IM_PI * 1.00f }, // Left
		{ ImVec2(-1, 0), ImVec2(1, 0), ImVec2(1, 1), IM_PI * 0.00f }, // Right
		{ ImVec2(0, +1), ImVec2(0, 0), ImVec2(1, 0), IM_PI * 1.50f }, // Up
		{ ImVec2(0, -1), ImVec2(1, 1), ImVec2(0, 1), IM_PI * 0.50f }  // Down
	};

	auto GetResizeBorderRect = [](ImGuiWindow* window, int border_n, float perp_padding, float thickness)
	{
		ImRect rect = window->Rect();
		if (thickness == 0.0f)
		{
			rect.Max.x -= 1;
			rect.Max.y -= 1;
		}
		if (border_n == ImGuiDir_Left) { return ImRect(rect.Min.x - thickness, rect.Min.y + perp_padding, rect.Min.x + thickness, rect.Max.y - perp_padding); }
		if (border_n == ImGuiDir_Right) { return ImRect(rect.Max.x - thickness, rect.Min.y + perp_padding, rect.Max.x + thickness, rect.Max.y - perp_padding); }
		if (border_n == ImGuiDir_Up) { return ImRect(rect.Min.x + perp_padding, rect.Min.y - thickness, rect.Max.x - perp_padding, rect.Min.y + thickness); }
		if (border_n == ImGuiDir_Down) { return ImRect(rect.Min.x + perp_padding, rect.Max.y - thickness, rect.Max.x - perp_padding, rect.Max.y + thickness); }
		IM_ASSERT(0);
		return ImRect();
	};


	ImGuiContext& g = *GImGui;
	float rounding = window->WindowRounding;
	float border_size = 1.0f; // window->WindowBorderSize;
	if (border_size > 0.0f && !(window->Flags & ImGuiWindowFlags_NoBackground))
		window->DrawList->AddRect(window->Pos, { window->Pos.x + window->Size.x,  window->Pos.y + window->Size.y }, ImGui::GetColorU32(ImGuiCol_Border), rounding, 0, border_size);

	int border_held = window->ResizeBorderHeld;
	if (border_held != -1)
	{
		const ImGuiResizeBorderDef& def = resize_border_def[border_held];
		ImRect border_r = GetResizeBorderRect(window, border_held, rounding, 0.0f);
		ImVec2 p1 = ImLerp(border_r.Min, border_r.Max, def.SegmentN1);
		const float offsetX = def.InnerDir.x * rounding;
		const float offsetY = def.InnerDir.y * rounding;
		p1.x += 0.5f + offsetX;
		p1.y += 0.5f + offsetY;

		ImVec2 p2 = ImLerp(border_r.Min, border_r.Max, def.SegmentN2);
		p2.x += 0.5f + offsetX;
		p2.y += 0.5f + offsetY;

		window->DrawList->PathArcTo(p1, rounding, def.OuterAngle - IM_PI * 0.25f, def.OuterAngle);
		window->DrawList->PathArcTo(p2, rounding, def.OuterAngle, def.OuterAngle + IM_PI * 0.25f);
		window->DrawList->PathStroke(ImGui::GetColorU32(ImGuiCol_SeparatorActive), 0, ImMax(2.0f, border_size)); // Thicker than usual
	}
	if (g.Style.FrameBorderSize > 0 && !(window->Flags & ImGuiWindowFlags_NoTitleBar) && !window->DockIsActive)
	{
		float y = window->Pos.y + window->TitleBarHeight() - 1;
		window->DrawList->AddLine(ImVec2(window->Pos.x + border_size, y), ImVec2(window->Pos.x + window->Size.x - border_size, y), ImGui::GetColorU32(ImGuiCol_PlotHistogram), g.Style.FrameBorderSize);
	}
}




void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void draw(GLFWwindow* window, ImGuiIO& io);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	editor.reCalculateBounds = true;
	glViewport(0, 0, width, height);
	draw(window, ImGui::GetIO());
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	for (int i = 0; i < count; i++) {
		if (std::filesystem::is_directory(paths[i])) {
			GL_INFO("Folder: {}", paths[i]);
		} else {
			GL_INFO("File: {}", paths[i]);
			std::ifstream t(paths[i]);
			t.seekg(0, std::ios::end);
			size = t.tellg();
			file_data.resize(size, ' ');
			t.seekg(0);
			t.read(&file_data[0], size);
			editor.SetBuffer(file_data);
		}
	}
}

void draw(GLFWwindow* window, ImGuiIO& io)
{
	static bool isFileLoaded = false;
	if (!isFileLoaded) {
		std::ifstream t("D:/Projects/c++/txedit/src/TextEditor.cpp");
		t.seekg(0, std::ios::end);
		size = t.tellg();
		file_data.resize(size, ' ');
		t.seekg(0);
		t.read(&file_data[0], size);
		editor.SetBuffer(file_data);
		isFileLoaded = true;
	}
	glfwPollEvents();
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	glfwSetDropCallback(window, drop_callback);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	if (!showCustumTitleBar)
		window_flags |= ImGuiWindowFlags_MenuBar;

	const bool isMaximized = IsMaximized(window);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f,0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);

	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
	ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
	ImGui::PopStyleColor(); // MenuBarBg
	ImGui::PopStyleVar(2);

	ImGui::PopStyleVar(2);
	{
		ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(100, 100, 100, 255));
		// Draw window border if the window is not maximized
		if (!isMaximized)
			RenderWindowOuterBorders(ImGui::GetCurrentWindow());

		ImGui::PopStyleColor(); // ImGuiCol_Border
	}

	if (showCustumTitleBar)
	{
		float titleBarHeight=0.0f;
		UI_DrawTitlebar(titleBarHeight);
		ImGui::SetCursorPosY(titleBarHeight);
	}


	ImGuiStyle& style = ImGui::GetStyle();
	float minWinSizeX = style.WindowMinSize.x;
	style.WindowMinSize.x = 370.0f;
	ImGui::DockSpace(ImGui::GetID("MyDockspace"),{0,0});
	style.WindowMinSize.x = minWinSizeX;

	// if (!showCustumTitleBar){
	// 	// UI_DrawMenubar();
	// 	if (ImGui::BeginMenuBar()) {
	// 		if (ImGui::BeginMenu("File")) {
	// 			ImGui::MenuItem("New File");
	// 			ImGui::MenuItem("Open File");
	// 			ImGui::MenuItem("Open Folder");
	// 			ImGui::EndMenu();
	// 		}

	// 		if (ImGui::BeginMenu("Edit")) {
	// 			ImGui::MenuItem("Cut");
	// 			ImGui::MenuItem("Copy");
	// 			ImGui::MenuItem("Paste");
	//             ImGui::EndMenu();
	// 		}
	// 		ImGui::EndMenuBar();
	// 	}
	// }



	ImGui::End();
	#ifdef GL_DEBUG
	ImGui::ShowDemoWindow();


	ImGui::Begin("Project");


	static int LineSpacing = 10.0f;
	if (ImGui::SliderInt("LineSpacing", &LineSpacing, 0, 20)) {
		editor.setLineSpacing(LineSpacing);
	}

    ImGui::Spacing();
	ImGui::Text("Position:%.2f  %.2f", ImGui::GetMousePos().x,ImGui::GetMousePos().y);
    ImGui::Spacing();
    ImGui::Text("mCursorPosition: X:%d  Y:%d",editor.GetEditorState()->mCursorPosition.mColumn,editor.GetEditorState()->mCursorPosition.mLine);
    ImGui::Text("mSelectionStart: X:%d  Y:%d",editor.GetEditorState()->mSelectionStart.mColumn,editor.GetEditorState()->mSelectionStart.mLine);
    ImGui::Text("mSelectionEnd:   X:%d  Y:%d",editor.GetEditorState()->mSelectionEnd.mColumn,editor.GetEditorState()->mSelectionEnd.mLine);


    ImGui::Spacing();
    ImGui::Spacing();
    static std::string mode;
    ImColor color;
    switch(editor.GetSelectionMode()){
	    case 0: 
	    	mode="Normal";
	    	color=ImColor(50,206,187,255);
	    	break;
	    case 1: 
	    	mode="Word";
	    	color=ImColor(233,196,106,255);
	    	break;
	    case 2: 
	    	mode="Line";
	    	color=ImColor(231,111,81,255);
	    	break;
    }
    ImGui::Text("SelectionMode: ");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text,color.Value);
    ImGui::Text("%s", mode.c_str());
    ImGui::PopStyleColor();



    ImGui::Spacing();
    ImGui::Spacing();
    static int v{1};
    ImGui::Text("Goto Line: "); ImGui::SameLine(); 
    if(ImGui::InputInt("##ScrollToLine", &v,1,100,ImGuiInputTextFlags_EnterReturnsTrue))
        editor.ScrollToLineNumber(v);

    static bool isImageLoaded=false;
    static OpenGL::Image img;
    if(!isImageLoaded){
    	img.LoadFromBuffer(window_logo,sizeof(window_logo)/sizeof(window_logo[0]));
    	isImageLoaded=true;
    }

    if(isImageLoaded) ImGui::Image((void*)(intptr_t)img.GetTexture(),{static_cast<float>(img.GetWidth()),static_cast<float>(img.GetHeight())});

	ImGui::End();
	#endif

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetContentRegionMax().x + 1500.0f, 0));
	ImGui::Begin("Editor", 0, ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::PopStyleVar();

	if (size) {
		ImGui::PushFont(io.Fonts->Fonts[1]);
        editor.render();
		ImGui::PopFont();
	}
	ImGui::End();

	#ifdef GL_DEBUG
	ImGui::Begin("Console");
	ImGui::End();
	#endif

	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}

	glfwMakeContextCurrent(window);
	glfwSwapBuffers(window);
}


int main(void)
{
#ifdef GL_DEBUG
	OpenGL::Log::Init();
#endif

	if (!glfwInit()) return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
	// glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); //Disables Titlebar and no events received
	// glfwWindowHint(GLFW_TITLEBAR,!showCustumTitleBar);

	window = glfwCreateWindow(WIDTH, HEIGHT, "TxEdit", NULL, NULL);
	glfwSetWindowSizeLimits(window, 330, 500, GLFW_DONT_CARE, GLFW_DONT_CARE);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	glfwShowWindow(window);

	glfwMakeContextCurrent(window);

	GLFWimage images[1];
	images[0].pixels = stbi_load_from_memory(logo_img, IM_ARRAYSIZE(logo_img), &images[0].width, &images[0].height, 0, 4); // rgba channels
	glfwSetWindowIcon(window, 1, images);
	stbi_image_free(images[0].pixels);

	// glfwSetWindowUserPointer(window, this);
	glfwSetTitlebarHitTestCallback(window, [](GLFWwindow* window, int x, int y, int* hit)
	{
		*hit = isTitleBarHovered;
	});


	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();


	GL_INFO("Initializing Fonts");
	io.Fonts->Clear();
	// io.IniFilename=nullptr;
	io.LogFilename = nullptr;

	glfwSwapInterval(1);
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 2.0f;
	style.ItemSpacing.y = 6.0f;
	style.ScrollbarRounding = 2.0f;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}


	ImGui_ImplGlfw_InitForOpenGL(window, true);
	if (!ImGui_ImplOpenGL2_Init()) GL_ERROR("Failed to initit OpenGL 2");

	// glfwSetKeyCallback(window, keyboardCallback);

	static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	ImFontConfig icon_config;
	icon_config.MergeMode = true;
	icon_config.PixelSnapH = true;
	icon_config.FontDataOwnedByAtlas = false;

	const int font_data_size = IM_ARRAYSIZE(data_font);
	const int icon_data_size = IM_ARRAYSIZE(data_icon);

	ImFontConfig font_config;
	font_config.FontDataOwnedByAtlas = false;
	io.Fonts->AddFontFromMemoryTTF((void*)data_font, font_data_size, 16, &font_config);
	io.Fonts->AddFontFromMemoryTTF((void*)data_icon, icon_data_size, 20 * 2.0f / 3.0f, &icon_config, icons_ranges);

	io.Fonts->AddFontFromMemoryTTF((void*)monolisa_medium, IM_ARRAYSIZE(monolisa_medium), 12, &font_config);


	StyleColorsDracula();

	while (!glfwWindowShouldClose(window)) {
		draw(window, io);
	}
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}