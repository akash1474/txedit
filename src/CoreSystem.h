#include "string"
#include "vector"
#include "imgui.h"
#include "GLFW/glfw3.h"

class CoreSystem{
	const char* currFile;
	GLFWwindow* m_Window{0};

	
	void renderFile();
};