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
	static inline void (*mCallbackFnEx)(const char* data1,const char* data2);

	static inline bool mIsCallbackEx=false;
	static inline char mInputTextBuffer[1024];


	void Init(Editor* editorPtr);


	void Render(ImVec2& size,const ImGuiViewport* viewport);
	void RenderInputPanel(ImVec2& size,const ImGuiViewport* viewport);

	void ShowNotification(const char* title,const char* info,NotificationType type=NotificationType::Info);
	void SetFileType(const char* filetype);
	void SetCursorCoordinate(const Coordinates& cursorPosition);
	bool IsInputPanelOpen();
	void ShowInputPanel(const char* title,void(*callback)(const char*),const char* placeholder=nullptr,bool showButton=false,const char* btnName="Done");
	void ShowInputPanelEx(const char* title,void(*callback)(const char*,const char*),const char* placeholder=nullptr,bool showButton=false,const char* btnName="Done");
	void CloseInputPanel();
};
