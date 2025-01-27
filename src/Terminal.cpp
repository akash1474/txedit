#include "Coordinates.h"
#include "DataTypes.h"
#include "GLFW/glfw3.h"
#include "Log.h"
#include "Timer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "pch.h"
#include <handleapi.h>
#include <mutex>
#include "Terminal.h"
#include "regex"


// TODO:
// Separate read output to different lines and implement selection copy + rightclick copy
// Keyboard input
// UTF-8 support
// Establish all default terminal behaviour

static bool IsUTFSequence(char c) { return (c & 0xC0) == 0x80; }

static int UTF8CharLength(uint8_t c)
{
	if ((c & 0xFE) == 0xFC)
		return 6;
	if ((c & 0xFC) == 0xF8)
		return 5;
	if ((c & 0xF8) == 0xF0)
		return 4;
	else if ((c & 0xF0) == 0xE0)
		return 3;
	else if ((c & 0xE0) == 0xC0)
		return 2;
	return 1;
}


Terminal::Terminal() : mIsCommandRunning(false), mScrollToBottom(false), mReaderThreadIsRunning(false)
{
	assert(mConPTY.Initialize());
	assert(mConPTY.CreateAndStartProcess(L"cmd.exe chcp 65001"));
	StartShell();
}

Terminal::~Terminal() { CloseShell(); }

std::string RemoveANSISequences(const std::string& str)
{
	OpenGL::ScopedTimer("Terminal::ANSI Parsing");
	std::regex ansiRegex(R"(\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~]))");
	std::regex positioningRegex(R"(\x1B\[\d+;\d+H)");
	std::string result = std::regex_replace(str, positioningRegex, "\n");


	std::string temp = result;
	result.clear();
	auto it = std::sregex_iterator(temp.begin(), temp.end(), ansiRegex);
	auto end = std::sregex_iterator();

	std::size_t lastPos = 0;

	for (; it != end; ++it) {
		std::smatch match = *it;
		result.append(temp, lastPos, match.position() - lastPos);
		lastPos = match.position() + match.length();
	}

	result.append(temp, lastPos, temp.size() - lastPos);

	std::regex pathRegex(R"(0;C:\\Windows\\SYSTEM32\\cmd\.exe)");
	result = std::regex_replace(result, pathRegex, "");

	return result;
}


void Terminal::Render()
{
	ImGui::Begin("Terminal");


	this->Draw();

	if (mScrollToBottom) {
		static int nFrameToExecute = 0;
		ImGuiContext& g = *GImGui;

		ImGui::SetScrollY(mWindow, mWindow->ScrollMax.y);
		if (nFrameToExecute > 3) {
			mScrollToBottom = false;
			nFrameToExecute = 0;
		} else
			nFrameToExecute++;
	}


	if ((glfwGetTime() - mPrevTime > 0.1f) && mIsCommandRunning) {
		GL_INFO("Command Running Completed");
		mIsCommandRunning = false;
		mReadOnlyCoords = GetCommandInsertPosition();
	}

	if (ImGui::IsWindowHovered())
		this->HandleInputs();

	ImGui::End();
}

void Terminal::StartShell()
{
	mReaderThreadIsRunning = true;
	mReaderThread = std::async(std::launch::async, &Terminal::ShellReader, this);
}

void Terminal::ShellReader()
{
	static std::string buffer;
	while (mReaderThreadIsRunning) {
		GL_INFO("Thread Running");
		buffer = mConPTY.ReadOutput();
		if (!buffer.empty()) {
			mBuffer += RemoveANSISequences(buffer);
			if (!mBuffer.empty() && mBuffer.back() == '\x07')
				mBuffer.pop_back();
			// std::lock_guard<std::mutex> lock(mOutputMutex);
			// mDisplayBuffer=mBuffer;
			GL_WARN(mBuffer);
			this->AppendText(mBuffer);
			mBuffer.clear();
			mScrollToBottom = true;
			mIsCommandRunning = true;
			mRecalculateBounds = true;
		}
		mPrevTime = glfwGetTime();
		buffer.clear();
	}
}


void Terminal::CloseShell()
{
	std::string cmd="exit";
	mReaderThreadIsRunning = false;
	mConPTY.WriteInput(cmd);
    // Wait for the reader thread to finish
    if (mReaderThread.valid()) {
        try {
            mReaderThread.get(); // Wait for the thread to complete
        } catch (const std::exception& e) {
            GL_ERROR("Terminal::CloseShell: Exception while joining thread: {}", e.what());
        }
    }

    // Cleanup the ConPTY instance
    mConPTY.CloseShell();
}

void Terminal::RunCommand(std::string& command)
{
	if (mIsCommandRunning)
		return;
	if (command == "exit") {
		mState.mCursorPosition = GetCommandInsertPosition();
		return;
	} else if (command.empty()) {
		std::string& line = mLines[mReadOnlyCoords.mLine];
		int idx = GetCharacterIndex(mReadOnlyCoords);
		mLines.push_back(line.substr(0, idx));
		mState.mCursorPosition = GetCommandInsertPosition();
		mReadOnlyCoords = mState.mCursorPosition;
		mRecalculateBounds = true;
		mScrollToBottom = true;
		return;
	} else if (command == "cls") {
		std::string& line = mLines[mReadOnlyCoords.mLine];
		std::string dir = line.substr(0, GetCharacterIndex(mReadOnlyCoords));
		mBuffer.clear();
		mLines.clear();
		mLines.push_back(dir);
		GL_INFO(mLines[0]);
		mState.mCursorPosition = GetCommandInsertPosition();
		// FixMe: Will cause error if the directory contains multiByte characters
		mReadOnlyCoords = {0, (int)dir.size()};
		ImGui::SetScrollY(mWindow, 0);

		return;
	}


	GL_WARN("Terminal::CMD:'{}'", command.c_str());
	PushHistory(command);
	command += "\r\n";
	mConPTY.WriteInput(command);
}

void Terminal::UpdateBounds()
{
	GL_WARN("UPDATING BOUNDS");
	mPosition = mWindow->Pos;
	GL_INFO("EditorPosition: x:{} y:{}", mPosition.x, mPosition.y);

	mSize = ImVec2(mWindow->ContentRegionRect.Max.x, mLines.size() * (mLineSpacing + mCharacterSize.y) + 0.5 * mLineSpacing);
	mBounds = ImRect(mPosition, ImVec2(mPosition.x + mSize.x, mPosition.y + mSize.y));

	mRecalculateBounds = false;
}

float Terminal::GetSelectionPosFromCoords(const Coordinates& coords) const
{
	float offset{0.0f};
	if (coords == mState.mSelectionStart)
		offset = -1.0f;
	return mLinePosition.x - offset + (coords.mColumn * mCharacterSize.x);
}

Coordinates Terminal::GetCommandInsertPosition()
{
	if (mLines.empty())
		return {0, 0};
	int aLine = mLines.size() - 1;
	int aColumn = GetCharacterColumn(aLine, mLines[aLine].size());
	return {aLine, aColumn};
}

int Terminal::GetCharacterColumn(int aLine, int aIndex) const
{
	if (aLine >= mLines.size())
		return 0;
	auto& line = mLines[aLine];
	int col = 0;
	int i = 0;
	while (i < aIndex && i <= (int)line.size()) {
		auto c = line[i];
		i += UTF8CharLength(c);
		if (c == '\t')
			col = (col / mTabWidth) * mTabWidth + mTabWidth;
		else
			col++;
	}
	return col;
}


int Terminal::GetCharacterIndex(const Coordinates& aCoordinates) const
{
	if (aCoordinates.mLine >= mLines.size())
		return -1;
	auto& line = mLines[aCoordinates.mLine];
	int c = 0;
	int i = 0;
	for (; i < line.size() && c < aCoordinates.mColumn;) {
		if (line[i] == '\t')
			c = (c / mTabWidth) * mTabWidth + mTabWidth;
		else
			++c;
		i += UTF8CharLength(line[i]);
	}
	return i;
}


size_t Terminal::GetLineMaxColumn(int aLine) const
{
	if (aLine >= mLines.size())
		return 0;
	auto& line = mLines[aLine];
	int col = 0;
	for (unsigned i = 0; i < line.size();) {
		auto c = line[i];
		if (c == '\t')
			col = (col / mTabWidth) * mTabWidth + mTabWidth;
		else
			col++;
		i += UTF8CharLength(c);
	}
	return col;
}


void Terminal::AppendText(const std::string& text)
{
	Coordinates aWhere = GetCommandInsertPosition();
	int idx = GetCharacterIndex(aWhere);
	if (idx == -1)
		idx = 0;

	size_t foundIndex = text.find('\n');
	bool isMultiLineText = foundIndex != std::string::npos;
	std::string segment = text.substr(0, foundIndex);

	if (!isMultiLineText) {
		if (mLines.empty())
			mLines.push_back(segment);
		else
			mLines[aWhere.mLine].insert(idx, text);
		mState.mCursorPosition.mColumn += text.size();
		return;
	}


	// Inserting into currentline
	if (mLines.empty())
		mLines.push_back(segment);
	else
		mLines[aWhere.mLine] += segment;

	std::string line;
	int lineIndex = aWhere.mLine + 1;
	for (size_t i = (foundIndex + 1); i < text.size(); i++) {
		if (text[i] == '\r')
			continue;
		if (text[i] == '\n') {
			mLines.insert(mLines.begin() + lineIndex, line);
			line.clear();
			lineIndex++;
		} else
			line += text[i];
	}

	// last string
	mLines.insert(mLines.begin() + lineIndex, line);

	mState.mCursorPosition.mLine = lineIndex;
	mState.mCursorPosition.mColumn = GetLineMaxColumn(lineIndex);
}

std::pair<int, int> Terminal::GetIndexOfWordAtCursor(const Coordinates& coords) const
{

	int idx = GetCharacterIndex(coords);
	int start_idx{idx};
	int end_idx{idx};
	bool a = true, b = true;
	while ((a || b)) {
		char chr{0};
		if (start_idx == 0 || start_idx < 0) {
			a = false;
			start_idx = 0;
		} else
			chr = mLines[coords.mLine][start_idx - 1];


		if (a && (isalnum(chr) || chr == '_'))
			start_idx--;
		else
			a = false;

		if (end_idx >= mLines[coords.mLine].size()) {
			b = false;
			end_idx = mLines[coords.mLine].size();
		} else {
			chr = mLines[coords.mLine][end_idx];
			if (b && (isalnum(chr) || chr == '_'))
				end_idx++;
			else
				b = false;
		}
	}
	return {start_idx, end_idx};
}


void Terminal::Draw()
{

	static bool isInit = false;
	if (!isInit) {
		mWindow = ImGui::GetCurrentWindow();
		mPosition = mWindow->Pos;

		mCharacterSize = ImVec2(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr));

		// mLineBarMaxCountWidth=GetNumberWidth(mLines.size());
		// mLineBarWidth=ImGui::CalcTextSize(std::to_string(mLines.size()).c_str()).x + 2 * mLineBarPadding;

		mLinePosition = ImVec2({mPosition.x + mPaddingLeft, mPosition.y});
		mLineHeight = mLineSpacing + mCharacterSize.y;

		mTitleBarHeight = mWindow->TitleBarHeight;
		mSelectionMode = SelectionMode::Normal;

		GL_WARN("LINE HEIGHT:{}", mLineHeight);
		GL_WARN("TITLE HEIGHT:{}", mTitleBarHeight);
		isInit = true;
	}

	if (mPosition.x != mWindow->Pos.x || mPosition.y != mWindow->Pos.y)
		mRecalculateBounds = true;
	if (mRecalculateBounds)
		UpdateBounds();


	ImGuiIO& io = ImGui::GetIO();
	const ImGuiID id = ImGui::GetID("##Editor");


	ImGui::ItemSize(mBounds, 0.0f);
	if (!ImGui::ItemAdd(mBounds, id))
		return;

	if (ImGui::IsMouseDown(0) && mSelectionMode == SelectionMode::Word && (ImGui::GetMousePos().y > (mPosition.y + mWindow->Size.y))) {
		ImGui::SetScrollY(ImGui::GetScrollY() + mLineHeight);
		if (mState.mSelectionEnd.mLine < mLines.size() - 1) {
			mState.mSelectionEnd.mLine++;
			mState.mCursorPosition.mLine++;
		}
	}

	if (ImGui::IsMouseDown(0) && mSelectionMode == SelectionMode::Word && (ImGui::GetMousePos().y < mPosition.y)) {
		ImGui::SetScrollY(ImGui::GetScrollY() - mLineHeight);
		if (mState.mSelectionEnd.mLine > 0) {
			mState.mSelectionEnd.mLine--;
			mState.mCursorPosition.mLine--;
		}
	}

	// // BackGrounds
	// mWindow->DrawList->AddRectFilled({mPosition.x + mLineBarWidth, mPosition.y}, mBounds.Max,ImColor(29, 32, 33, 255)); // Code


	mMinLineVisible = fmax(0.0f, ImGui::GetScrollY() / mLineHeight);
	mLinePosition.y = (mPosition.y + mTitleBarHeight + (mState.mCursorPosition.mLine - floor(mMinLineVisible)) * mLineHeight);
	mLinePosition.x = mPosition.x + mPaddingLeft - ImGui::GetScrollX();


	// Highlight Selections
	if (ImGui::IsWindowFocused() && (mSelectionMode == SelectionMode::Word || mSelectionMode == SelectionMode::Line)) {
		Coordinates selectionStart = mState.mSelectionStart;
		Coordinates selectionEnd = mState.mSelectionEnd;

		if (selectionStart > selectionEnd)
			std::swap(selectionStart, selectionEnd);

		if (selectionStart.mLine == selectionEnd.mLine) {
			ImVec2 start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y);
			ImVec2 end(GetSelectionPosFromCoords(selectionEnd), mLinePosition.y + mLineHeight);

			mWindow->DrawList->AddRectFilled(start, end, ImColor(54, 51, 50, 255));
		} else if ((selectionStart.mLine + 1) == selectionEnd.mLine) { // Rendering selection two lines

			float prevLinePositonY = mLinePosition.y;
			if (mState.mCursorDirectionChanged) {
				mLinePosition.y = (mPosition.y + mTitleBarHeight + (selectionEnd.mLine - floor(mMinLineVisible)) * mLineHeight);
			}

			ImVec2 start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y - mLineHeight);
			ImVec2 end(mLinePosition.x + GetLineMaxColumn(selectionStart.mLine) * mCharacterSize.x + mCharacterSize.x, mLinePosition.y);

			mWindow->DrawList->AddRectFilled(start, end, ImColor(54, 51, 50, 255));


			start = {mLinePosition.x, mLinePosition.y};
			end = {GetSelectionPosFromCoords(selectionEnd), mLinePosition.y + mLineHeight};

			mWindow->DrawList->AddRectFilled(start, end, ImColor(54, 51, 50, 255));


			if (mState.mCursorDirectionChanged) {
				mLinePosition.y = prevLinePositonY;
			}
		} else if ((selectionEnd.mLine - selectionStart.mLine) > 1) { // Selection multiple lines
			int start = selectionStart.mLine + 1;
			int end = selectionEnd.mLine;
			int diff = end - start;


			float prevLinePositonY = mLinePosition.y;
			if (mState.mCursorDirectionChanged) {
				mLinePosition.y = (mPosition.y + mTitleBarHeight + (selectionEnd.mLine - floor(mMinLineVisible)) * mLineHeight);
			}

			ImVec2 p_start(GetSelectionPosFromCoords(selectionStart), mLinePosition.y - (diff + 1) * mLineHeight);
			ImVec2 p_end(mLinePosition.x + GetLineMaxColumn(selectionStart.mLine) * mCharacterSize.x + mCharacterSize.x,
			             mLinePosition.y - diff * mLineHeight);

			mWindow->DrawList->AddRectFilled(p_start, p_end, ImColor(54, 51, 50, 255));


			while (start < end) {
				diff = end - start;

				ImVec2 p_start(mLinePosition.x, mLinePosition.y - diff * mLineHeight);
				ImVec2 p_end(p_start.x + GetLineMaxColumn(start) * mCharacterSize.x + mCharacterSize.x,
				             mLinePosition.y - (diff - 1) * mLineHeight);

				mWindow->DrawList->AddRectFilled(p_start, p_end, ImColor(54, 51, 50, 255));
				start++;
			}


			p_start = {mLinePosition.x, mLinePosition.y};
			p_end = {GetSelectionPosFromCoords(selectionEnd), mLinePosition.y + mLineHeight};

			mWindow->DrawList->AddRectFilled(p_start, p_end, ImColor(54, 51, 50, 255));

			if (mState.mCursorDirectionChanged) {
				mLinePosition.y = prevLinePositonY;
			}
		}
	}


	int start = std::min(int(mMinLineVisible), (int)mLines.size());
	int lineCount = (mWindow->Size.y) / mLineHeight;
	int end = std::min(start + lineCount + 1, (int)mLines.size());


	int lineNo = 0;
	int i_prev = 0;

	// Rendering Lines and Vertical Indentation Lines
	while (start != end) {
		float linePosY = mPosition.y + (lineNo * mLineHeight) + mTitleBarHeight + (0.5 * mLineSpacing);
		mWindow->DrawList->AddText({mLinePosition.x, linePosY}, ImColor(235, 219, 178, 255), mLines[start].c_str());

		start++;
		lineNo++;
	}


	// Cursor
	if(ImGui::IsWindowFocused())
	{
		ImVec2 cursorPosition(mLinePosition.x - 1.0f + (mState.mCursorPosition.mColumn * mCharacterSize.x), mLinePosition.y);
		mWindow->DrawList->AddRectFilled(cursorPosition, {cursorPosition.x + 2.0f, cursorPosition.y + mLineHeight}, ImColor(255, 255, 255, 255));
	}


	start = std::min(int(mMinLineVisible), (int)mLines.size());
	lineCount = (mWindow->Size.y) / mLineHeight;
	end = std::min(start + lineCount + 1, (int)mLines.size());


	mLineHeight = mLineSpacing + mCharacterSize.y;
}


Coordinates Terminal::MapScreenPosToCoordinates(const ImVec2& mousePosition)
{
	Coordinates coords;

	float currentLineNo = (ImGui::GetScrollY() + (mousePosition.y - mPosition.y) - mTitleBarHeight) / mLineHeight;
	coords.mLine = std::max(0, (int)floor(currentLineNo - (mMinLineVisible - floor(mMinLineVisible))));
	if (coords.mLine > mLines.size() - 1)
		coords.mLine = mLines.size() - 1;

	coords.mColumn = std::max(0, (int)round((ImGui::GetScrollX() + mousePosition.x - mPosition.x - mPaddingLeft) / mCharacterSize.x));

	// Snapping to nearest tab char
	int col = 0;
	size_t i = 0;
	while (i < mLines[coords.mLine].size()) {
		if (mLines[coords.mLine][i] == '\t') {
			col += mTabWidth;
			if (coords.mColumn > (col - mTabWidth) && coords.mColumn < col) {
				coords.mColumn = col - mTabWidth;
				break;
			}
		} else
			col++;

		i++;
	}

	mState.mCursorPosition.mLine = coords.mLine;

	int lineLength = GetLineMaxColumn(mState.mCursorPosition.mLine);
	if (coords.mColumn > lineLength)
		coords.mColumn = lineLength;

	return coords;
}

std::string UnicodeToUTF8(UINT32 codePoint)
{
	char utf8[5] = {0};
	int length = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)&codePoint, 1, utf8, sizeof(utf8), nullptr, nullptr);
	return std::string(utf8, length);
}

void Terminal::HandleInputs()
{
	ImGuiIO& io = ImGui::GetIO();
	auto click = ImGui::IsMouseClicked(0);
	auto doubleClick = ImGui::IsMouseDoubleClicked(0);

	auto t = ImGui::GetTime();
	auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);


	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;


	// Left mouse button triple click
	if (tripleClick) {
		GL_INFO("TRIPLE CLICK");
		mSelectionMode = SelectionMode::Line;
		mState.mSelectionStart.mColumn = 0;
		mState.mSelectionEnd.mColumn = GetLineMaxColumn(mState.mCursorPosition.mLine);
		mState.mCursorPosition.mColumn = mState.mSelectionEnd.mColumn;
		mLastClick = -1.0f;
	}

	// Left mouse button double click
	else if (doubleClick) {
		mSelectionMode = SelectionMode::Word;
		mLastClick = (float)ImGui::GetTime();
		auto [start_idx, end_idx] = GetIndexOfWordAtCursor(mState.mCursorPosition);
		if (start_idx == end_idx)
			return;
		// int tabCount=GetTabCountsUptoCursor(mState.mCursorPosition);
		mState.mSelectionStart = Coordinates(mState.mCursorPosition.mLine, start_idx);
		mState.mSelectionEnd = Coordinates(mState.mCursorPosition.mLine, end_idx);

		mState.mCursorPosition = mState.mSelectionEnd;
	}
	// Left mouse button click
	else if (click) {
		GL_INFO("MOUSE CLICK");

		mState.mSelectionStart = mState.mSelectionEnd = mState.mCursorPosition = MapScreenPosToCoordinates(ImGui::GetMousePos());
		mSelectionMode = SelectionMode::Normal;

		mState.mCursorDirectionChanged = false;
		mLastClick = (float)ImGui::GetTime();
	}

	// Mouse Click And Dragging
	else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0)) {
		if ((ImGui::GetMousePos().y - mPosition.y) < mTitleBarHeight)
			return;

		io.WantCaptureMouse = true;

		mState.mCursorPosition = mState.mSelectionEnd = MapScreenPosToCoordinates(ImGui::GetMousePos());
		mSelectionMode = SelectionMode::Word;

		if (mState.mSelectionStart > mState.mSelectionEnd)
			mState.mCursorDirectionChanged = true;
	}
	
	io.WantCaptureKeyboard = true;
	io.WantTextInput = true;

	auto& cursorPosition = mState.mCursorPosition;
	int max = GetLineMaxColumn(GetCommandInsertPosition().mLine);

	if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
		auto& line = mLines[mReadOnlyCoords.mLine];
		size_t pos = GetCharacterIndex(mReadOnlyCoords);
		std::string command = line.substr(pos);
		line.erase(pos);
		RunCommand(command);
	} else if (!shift && ctrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
		GL_CRITICAL("INTERRUPT");
		mConPTY.SendInterrupt();
	}
	// Left
	else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
		if (mSelectionMode != SelectionMode::Normal)
			ExitSelectionMode();
		if (cursorPosition.mColumn > 0 && cursorPosition.mColumn <= max)
			cursorPosition.mColumn--;
	}
	// Right
	else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
		if (mSelectionMode != SelectionMode::Normal)
			ExitSelectionMode();
		if (cursorPosition.mColumn >= 0 && cursorPosition.mColumn < max)
			cursorPosition.mColumn++;
	}
	// Up
	else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && !mHistory.empty()) {
		PushHistoryCommand(true);
	}
	// Down
	else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && !mHistory.empty()) {
		PushHistoryCommand(false);
	}
	// Delete
	else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
		if (cursorPosition.mColumn >= mReadOnlyCoords.mColumn && cursorPosition.mColumn < max) {
			auto cindex = GetCharacterIndex(mState.mCursorPosition);
			auto& line = mLines[mState.mCursorPosition.mLine];

			auto d = UTF8CharLength(line[cindex]);
			while (d-- > 0 && cindex < (int)line.size()) line.erase(line.begin() + cindex);
		}
	}
	// Backspace
	else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
		if (cursorPosition.mColumn > mReadOnlyCoords.mColumn && cursorPosition.mColumn <= max) {
			int idx = GetCharacterIndex(cursorPosition);
			int cidx = idx - 1;

			std::string& line = mLines[cursorPosition.mLine];

			while (cidx > 0 && IsUTFSequence(line[cidx])) cidx--;

			while (cidx < line.size() && idx-- > cidx) {
				line.erase(line.begin() + cidx);
			}
			cursorPosition.mColumn--;
		}
	}
	// Copy
	else if (ctrl && shift && !alt && ImGui::IsKeyPressed(ImGuiKey_C)) {
		if (mSelectionMode == SelectionMode::Normal)
			return;

		Coordinates selectionStart = mState.mSelectionStart;
		Coordinates selectionEnd = mState.mSelectionEnd;

		if (selectionStart > selectionEnd)
			std::swap(selectionStart, selectionEnd);

		if (selectionStart.mLine == selectionEnd.mLine) {
			uint32_t start = GetCharacterIndex(selectionStart);
			uint32_t end = GetCharacterIndex(selectionEnd);
			uint8_t word_len = end - start;

			std::string selection = mLines[mState.mCursorPosition.mLine].substr(start, word_len);
			ImGui::SetClipboardText(selection.c_str());
		}

		else {

			std::string copyStr;

			// start
			uint8_t start = GetCharacterIndex(selectionStart);
			uint8_t end = mLines[selectionStart.mLine].size();

			uint8_t word_len = end - start;
			copyStr += mLines[selectionStart.mLine].substr(start, word_len);
			copyStr += '\n';

			int startLine = selectionStart.mLine + 1;

			while (startLine < selectionEnd.mLine) {
				copyStr += mLines[startLine];
				copyStr += '\n';
				startLine++;
			}

			// end
			start = 0;
			end = GetCharacterIndex(selectionEnd);
			word_len = end - start;

			copyStr += mLines[selectionEnd.mLine].substr(start, word_len);

			ImGui::SetClipboardText(copyStr.c_str());
		}
	}
	// Paste
	else if (ctrl && shift && !alt && ImGui::IsKeyPressed(ImGuiKey_V)) {
		std::string text{ImGui::GetClipboardText()};
		if (!text.empty())
			AppendText(text);
	}

	// Character Input
	if (!io.InputQueueCharacters.empty()) {

		if (mSelectionMode != SelectionMode::Normal)
			mSelectionMode = SelectionMode::Normal;

		for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
			auto c = io.InputQueueCharacters[i];
			if (c != 0 && (c == '\n' || c >= 32)) {
				Coordinates coords = GetCommandInsertPosition();
				mState.mCursorPosition = coords;
				if (c > 127) {
					std::string grapheme = UnicodeToUTF8(c);
					GL_INFO("UNICODE:{}", grapheme);
					mLines[coords.mLine] += grapheme;
				} else {
					mLines[coords.mLine] += c;
				}
				mState.mCursorPosition.mColumn++;
			}
			GL_INFO("{}", (char)c);
		}


		io.InputQueueCharacters.resize(0);
	}
}


void Terminal::PushHistory(std::string& cmd)
{
	cmd.erase(std::find_if(cmd.rbegin(), cmd.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), cmd.end());
	if (!mHistory.empty()) {
		auto it = mHistory.begin();
		while (it != mHistory.end()) {
			if (*it == cmd) {
				it = mHistory.erase(it);
				break;
			} else
				it++;
		}
	}

	mHistory.push_back(cmd);
	mHistoryIterator = mHistory.end();
}

void Terminal::PushHistoryCommand(bool keyUp)
{
	if (keyUp && mHistoryIterator != mHistory.begin())
		mHistoryIterator--;
	else if (!keyUp && mHistoryIterator != --mHistory.end())
		mHistoryIterator++;

	mLines[mReadOnlyCoords.mLine].erase(GetCharacterIndex(mReadOnlyCoords));
	mLines[mReadOnlyCoords.mLine] += *mHistoryIterator;
	mState.mCursorPosition = GetCommandInsertPosition();
	mScrollToBottom = true;
}
