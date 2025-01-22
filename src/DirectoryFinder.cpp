#include "pch.h"
#include "Timer.h"
#include "imgui.h"
#include <mutex>
#include <regex>
#include "DirectoryFinder.h"
#include "TabsManager.h"


void DirectoryFinder::Setup(const std::string& aFolderPath, bool aOpenedFromExplorer)
{
	Get().mIsWindowOpen=true;
	Get().mOpenedFromExplorer=aOpenedFromExplorer;
	if(!aFolderPath.empty())
	{
		strcpy_s(Get().mDirectoryPath,aFolderPath.c_str());
	}
}


bool DirectoryFinder::Render()
{
	if(!Get().mIsWindowOpen)
		return false;

	ImGui::SetNextWindowDockID(Get().mDockspaceId,ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Directory Finder", &Get().mIsWindowOpen, ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::Text("Folder:");ImGui::SameLine();
		ImGui::InputText("##df_folder", Get().mDirectoryPath, MAX_PATH_LENGTH);

		ImGui::Checkbox("Regular expression", &Get().mRegexEnabled);
		ImGui::Checkbox("Case sensitive", &Get().mCaseSensitiveEnabled);

		ImGui::Text("To Find:");ImGui::SameLine();
		bool startSearching=ImGui::InputText("##df_tofind", Get().mToFind, INPUT_BUFFER_SIZE,ImGuiInputTextFlags_EnterReturnsTrue);

		if (Get().mFinderThread == nullptr)
		{
			if ((ImGui::Button("Find") || startSearching) && !std::string(Get().mToFind).empty()){
				Get().mFinderThread = new std::thread(&DirectoryFinder::Find);
			}
		}
		else
		{
			if (ImGui::Button("Cancel search"))
				Get().mFinderThread = nullptr;

			ImGui::PushStyleColor(ImGuiCol_PlotHistogram,IM_COL32(92, 0, 255, 255));
			ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-1.0f, 10.0f));
			ImGui::PopStyleColor();
		}



		ImGui::Separator();

		{
			std::lock_guard<std::mutex> guard(Get().mFinderMutex);
			for (int i = 0; i < Get().mResultFiles.size(); i++)
			{
				Get().mResultsInFile.clear();
				DFResultFile& file = Get().mResultFiles[i];
				for (int j = 0; j < file.results.size(); j++)
				{
					DFResult& res = file.results[j];
					std::smatch resultFilterMatch;
					Get().mResultsInFile.push_back(&res);
				}

				if (Get().mResultsInFile.size() > 0)
				{
					ImGui::Separator();
					ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyle().Colors[ImGuiCol_TextLink]);
					ImGui::TextUnformatted(file.fileName.c_str());
					ImGui::PopStyleColor();
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
					for (DFResult* res : Get().mResultsInFile)
						if (ImGui::Selectable(std::string(res->displayText + "##" + file.fileName).c_str(),false))
						{
							GL_INFO("GoTo Match:{} {}",res->lineNumber,file.fileName);
							TabsManager::OpenFileWithAtLineNumber(file.filePath, res->lineNumber-1,res->startCharIndex,res->endCharIndex);
						}

					ImGui::PopStyleColor();
				}
			}
		}
	}
	ImGui::End();


	// Joining the thread when user closes the window
	if (!Get().mIsWindowOpen && Get().mFinderThread != nullptr)
	{
		std::thread* threadToJoin = Get().mFinderThread;
		Get().mFinderThread = nullptr;
		threadToJoin->join();
	}
	return Get().mIsWindowOpen;
}



void DirectoryFinder::Find()
{
	OpenGL::ScopedTimer timer("DirectoryFinder::Find");


	std::string toFindAsStdString = std::string(Get().mToFind);
	std::regex toFindAsPattern;
	if (Get().mRegexEnabled)
	{
		try { toFindAsPattern = Get().mCaseSensitiveEnabled ? std::regex(Get().mToFind) : std::regex(Get().mToFind, std::regex_constants::icase); }
		catch (...) { std::cout << "[DirectoryFinder] Invalid regex given\n"; Get().mFinderThread = nullptr; return; }
	}

	{
		std::lock_guard<std::mutex> lock(Get().mFinderMutex);
		Get().mResultFiles.clear();
	}

	bool foundInFile = false;
	for (std::filesystem::recursive_directory_iterator i(StringToWString(Get().mDirectoryPath)), end; i != end; ++i)
	{
		if (Get().mFinderThread == nullptr)
			break;
		if (is_directory(i->path()))
			continue;

		foundInFile = false;

		std::string fileName = i->path().filename().generic_u8string();
		std::string filePath = i->path().generic_u8string();

		std::ifstream fileInput;
		fileInput.open(StringToWString(filePath));
		std::string line;
		int curLine = 0;
		while (std::getline(fileInput, line))
		{
			if (Get().mFinderThread == nullptr)
				break;
			curLine++;
			if (!Get().mRegexEnabled)
			{
				if (Get().mCaseSensitiveEnabled)
				{
					int startChar = line.find(Get().mToFind, 0);
					if (startChar != std::string::npos)
					{
						std::lock_guard<std::mutex> guard(Get().mFinderMutex);
						if (!foundInFile)
						{
							Get().mResultFiles.push_back({ filePath, fileName, {} });
							foundInFile = true;
						}
						Get().mResultFiles.back().results.push_back({ std::to_string(curLine) + ": " + line, curLine, startChar, startChar + (int)toFindAsStdString.length()});
					}
				}
				else // caseSensitiveEnabled
				{
					auto it = std::search(
						line.begin(), line.end(),
						toFindAsStdString.begin(), toFindAsStdString.end(),
						[](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
					);
					if (it != line.end())
					{
						std::lock_guard<std::mutex> guard(Get().mFinderMutex);
						int startChar = it - line.begin();
						if (!foundInFile)
						{
							Get().mResultFiles.push_back({ filePath, fileName, {} });
							foundInFile = true;
						}
						Get().mResultFiles.back().results.push_back({ std::to_string(curLine) + ": " + line, curLine, startChar, startChar + (int)toFindAsStdString.length() });
					}
				}
			}
			else // regexEnabled
			{
				std::smatch lineMatch;
				if (std::regex_search(line, lineMatch, toFindAsPattern))
				{
					std::lock_guard<std::mutex> guard(Get().mFinderMutex);
					if (!foundInFile)
					{
						Get().mResultFiles.push_back({ filePath, fileName, {} });
						foundInFile = true;
					}
					Get().mResultFiles.back().results.push_back({ std::to_string(curLine) + ": " + line, curLine, (int)lineMatch.position(), (int)(lineMatch.position() + lineMatch.length()) });
				}
			}
		}
		fileInput.close();
	}

	Get().mFinderThread = nullptr;
}






