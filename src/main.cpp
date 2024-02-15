#include "images.h"
#include "pch.h"
#include "CoreSystem.h"



int main(int argc,char* argv[])
{

	if(argc > 1) CoreSystem::HandleArguments(argc,argv);
	if(!CoreSystem::Init()) return -1;

	CoreSystem::SetApplicationIcon(logo_img,IM_ARRAYSIZE(logo_img));

	// Initialize ImGUI
	if(!CoreSystem::InitImGui()) return -1;
	CoreSystem::InitFonts();

   StyleColorDarkness();

	CoreSystem::GetTextEditor()->LoadFile("D:/Projects/c++/txedit/src/TextEditor.cpp");
	CoreSystem::GetFileNavigation()->AddFolder("D:/Projects/c++/txedit");

	while (!glfwWindowShouldClose(CoreSystem::GetGLFWwindow())) {
		CoreSystem::Draw();
	}

	CoreSystem::Destroy();
	return 0;
}
