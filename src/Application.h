#include "GLFW/glfw3.h"
#include "CoreSystem.h"
#include <future>

class Application{
	GLFWwindow* mWindow{0};
	int width=1200;
	int height=800;
	CoreSystem* mCoreSystem;

public:
	Application(const Application&)=delete;
	~Application(){}

	static Application& Get(){
		static Application instance;
		return instance;
	}

	static void Draw();
	static bool Init();
	static bool InitImGui();
	static bool InitFonts();
	static CoreSystem* GetCoreSystem(){return Get().mCoreSystem;}
	static void SetApplicationIcon(unsigned char* img,int length);
	void BackupDataBeforeCrash(); // Unimplemented still working but making a commit

	static void HandleCrash(int signal);
	static void SetupSystemSignalHandling();

	// Update TitleBar Color based on the width and current UserInterface/Page
	static void UpdateTitleBarColor();

	static void Destroy();
	static void HandleArguments(std::wstring args);
	static void CenterWindowOnScreen();
	static GLFWwindow* GetGLFWwindow(){return Get().mWindow;}


private:
	Application(){mCoreSystem=&CoreSystem::Get();};
};