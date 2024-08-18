#include "vector"
#include "string"
#include "uuid_v4.h"
#include "Log.h"
#include "imgui.h"

struct FileTab{
	std::string filepath;
	std::string filename;
	bool isTemp=false;
	bool isActive=false;
	bool isSaved=false;
	std::string id;
	FileTab(std::string path,std::string file,bool temp,bool active,bool save,std::string idx)
		:filepath(path),filename(file),isTemp(temp),isActive(active),isSaved(save),id(idx){}
};

class TabsManager{
	std::vector<FileTab> mTabs;


public:
	static TabsManager& Get(){
		static TabsManager instance;
		return instance;
	}

	static bool OpenFile(std::string filepath,bool isTemp=true);
	static void Render(ImGuiWindowClass& window_class,int winFlags);
	static bool RenderTab(std::vector<FileTab>::iterator tab,bool& removeTab);

private:
	TabsManager(){}
};