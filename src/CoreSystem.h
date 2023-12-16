#pragma once
#include "string"
#include "vector"
#include "imgui.h"
#include "GLFW/glfw3.h"
#include "TextEditor.h"
#include "DirectoryHandler.h"
#include "FileNavigation.h"
#include "StatusBarManager.h"

class CoreSystem{
	const char* currFile;
	GLFWwindow* m_Window{0};
	Editor mTextEditor;
	FileNavigation mFileNavigation;

public:
	CoreSystem(){ 
		mFileNavigation.SetTextEditor(&mTextEditor);
		StatusBarManager::Init(&mTextEditor,&mFileNavigation);
	}

	Editor* GetTextEditor(){return &mTextEditor;}
	FileNavigation* GetFileNavigation(){return &mFileNavigation;}
	void SetGLFWwindow(GLFWwindow* win){this->m_Window=win;}

	
	void Render();

	#ifdef GL_DEBUG
	void RenderDebugInfo();
	#endif
};