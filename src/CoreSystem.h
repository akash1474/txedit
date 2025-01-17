#pragma once
#include "GLFW/glfw3.h"
#include "DirectoryHandler.h"
#include "StatusBarManager.h"
#include "Terminal.h"


class CoreSystem{
	const char* mCurrentFile;
	GLFWwindow* mWindow{0};
	Terminal mTerminal;
	ImGuiID mLeftDockSpaceId=-1;
	ImGuiID mRightDockSpaceId=-1;
	ImGuiID mDockSpaceId=-1;

public:
	static ImGuiID GetMainDockSpaceID(){ return Get().mDockSpaceId;}
	static ImGuiID GetLeftMainDockSpaceID(){ return Get().mLeftDockSpaceId;}
	static ImGuiID GetRightMainDockSpaceID(){ return Get().mRightDockSpaceId;}
	CoreSystem(const CoreSystem&)=delete;

	static CoreSystem& Get(){
		static CoreSystem instance;
		return instance;
	}

	static GLFWwindow* GetGLFWwindow(){return Get().mWindow;}

	static void Render();
	static void InitFonts();
	static void HandleArguments(int argc,char* argv[]);
	static bool Init();
	static bool InitImGui();
	static void SetApplicationIcon(unsigned char* img,int length);
	static void Draw();
	static void Destroy();

	#ifdef GL_DEBUG
	static void RenderDebugInfo();
	#endif

private:
	CoreSystem(){ 
		StatusBarManager::Init();
	}

};

