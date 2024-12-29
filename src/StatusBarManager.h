#include "Animation.h"
#include "FileNavigation.h"
#include "imgui.h"
#include "string"
#include "Coordinates.h"
#include "TextEditor.h"

namespace StatusBarManager{

	enum class NotificationType{Info,Error,Warning,Success};

	const float PanelSize=38.0f;
	const float StatusBarSize=22.0f;


	static inline Editor* mTextEditor=nullptr;
	static inline FileNavigation* mFileNavigation=nullptr;

	//Notification
	static inline Animation mNotificationAnimation;
	static inline std::string mNotificationString;
	static inline ImVec4 mNotificationColor;
	static inline bool mDisplayNotification=false;
	ImVec4 GetNotificationColor(NotificationType type);

	//Panel
	static inline bool mIsInputPanelOpen=false;
	static inline std::string mPanelTitle="Title";
	static inline std::string mButtonTitle="Save";
	static inline std::string mPlaceholder;
	static inline bool mShowButton=false;
	static inline void (*mCallbackFn)(const char* data);


	void Init(Editor* editorPtr,FileNavigation* fileNavigation);


	void Render(ImVec2& size,const ImGuiViewport* viewport);
	void RenderInputPanel(ImVec2& size,const ImGuiViewport* viewport);

	FileNavigation* GetFileNavigation();


	void ShowNotification(const char* title,const char* info,NotificationType type=NotificationType::Info);
	void SetFileType(const char* filetype);
	void SetCursorCoordinate(const Coordinates& cursorPosition);
	bool IsInputPanelOpen();
	void ShowInputPanel(const char* title,void(*callback)(const char*),const char* placeholder=nullptr,bool showButton=false,const char* btnName="Done");
	void CloseInputPanel();
};
