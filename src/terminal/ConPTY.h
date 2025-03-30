#pragma once
#include <windows.h>
#include <string>

class ConPTY{
	HPCON hPC;
	HANDLE mHStdInWrite, mHStdOutRead,mHStdInRead,mHStdOutWrite;
    PROCESS_INFORMATION mProcessInfo;
    char mReadBuffer[4096];
    DWORD mBytesRead;

public:
	ConPTY();
	~ConPTY();

	bool Initialize();
	bool CreateAndStartProcess(const std::wstring& cmd);

	std::string ReadOutput();
	DWORD GetBytesRead() const noexcept{return mBytesRead;}
	void WriteInput(const std::string& str);


	void SendInterrupt();
	void CloseShell();

private:
	void CheckWriteFileErrors();
	void CheckReadFileErrors();
};