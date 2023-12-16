#include "string"
#include "vector"
#include "imgui.h"
#include "GLFW/glfw3.h"
#include "TextEditor.h"
#include "DirectoryHandler.h"

class CoreSystem{
	const char* currFile;
	GLFWwindow* m_Window{0};
	Editor mTextEditor;

	
	void Render();
};