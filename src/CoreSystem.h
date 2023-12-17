#pragma once
#include "string"
#include "vector"
#include "imgui.h"
#include "GLFW/glfw3.h"
#include "TextEditor.h"
#include "DirectoryHandler.h"
#include "FileNavigation.h"
#include "StatusBarManager.h"

#define WIDTH 900
#define HEIGHT 600

class CoreSystem{
	const char* mCurrentFile;
	GLFWwindow* mWindow{0};
	Editor mTextEditor;
	FileNavigation mFileNavigation;

public:
	CoreSystem(const CoreSystem&)=delete;

	static CoreSystem& Get(){
		static CoreSystem instance;
		return instance;
	}

	static Editor* GetTextEditor(){return &Get().mTextEditor;}
	static FileNavigation* GetFileNavigation(){return &Get().mFileNavigation;}
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
		mFileNavigation.SetTextEditor(&mTextEditor);
		StatusBarManager::Init(&mTextEditor,&mFileNavigation);
	}

};