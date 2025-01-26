#include "imgui.h"
#include "pch.h"
#include "QuickFileSearch.h"
#include "FileNavigation.h"
#include "TabsManager.h"

QuickFileSearch::~QuickFileSearch(){

}

std::vector<MatchResult> QuickFileSearch::ExecuteFuzzySearch(const std::vector<std::string>& aFileNames, const std::string& aQuery) {
	OpenGL::ScopedTimer timer("fuzzySearch");
    std::vector<MatchResult> results;

    for (const auto& filename : aFileNames) 
    {
        double score = CalculateFuzzyScore(filename, aQuery);
        if (score > 0.0)
            results.push_back({ filename, score });
    }

    std::sort(results.begin(), results.end());
    return std::move(results);
}

std::vector<std::string> QuickFileSearch::GetFilesInDirectory(const std::string& aDirectoryPath,const std::string& aRootPath, int aMaxDepth, int aCurrentDepth) {
    std::vector<std::string> files;
    if (aCurrentDepth > aMaxDepth) return files;
    
    for (const auto& entry : std::filesystem::directory_iterator(aDirectoryPath)) {
        if (entry.is_regular_file()) {
        	std::string ext=entry.path().extension().generic_string();
        	// if(ext == ".cpp" || ext == ".h" || ext==".c" || ext==".svg" || ext==".bat" || ext==".txt" || ext==".json" || ext==".md" || ext==".sln" || ext==".scm" )
            	files.push_back(std::filesystem::relative(entry.path(), aRootPath).generic_u8string());
        } else if (entry.is_directory()) {
        	std::string name=entry.path().filename().string();
        	if( name==".git" || name=="bin" || name==".cache" || name==".vs" )
        		continue;
            auto subFiles = GetFilesInDirectory(entry.path().string(),aRootPath, aMaxDepth, aCurrentDepth + 1);
            files.insert(files.end(), subFiles.begin(), subFiles.end());
        }
    }
    return std::move(files);
}


void QuickFileSearch::Render() {
	if(!Get().mIsOpen) return;
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg,IM_COL32(0, 0, 0, 50));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,IM_COL32(40, 40, 40, 255));

    ImGui::OpenPopup("Goto File");

    
	ImGui::SetNextWindowSize({600.0f,0});
    ImGui::SetNextWindowPos({(ImGui::GetMainViewport()->Size.x-600.0f)*0.5f,10.0f}, ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Goto File",0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar)) 
    {
    	int& selectedIndex = Get().mSelectedIndex;

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive())
   			ImGui::SetKeyboardFocusHere(0);
   		ImGui::SetNextItemWidth(-1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(4.0f,6.0f));
        if(ImGui::InputTextWithHint("##search","Search files by name...", Get().mSearchBuffer, sizeof(Get().mSearchBuffer)))
        {
	        Get().mMatchedResults.clear();
        	selectedIndex=0;
	        if (strlen(Get().mSearchBuffer) > 0) 
	        {
	        	Get().mMatchedResults=ExecuteFuzzySearch(Get().mFiles, Get().mSearchBuffer);
	        }
        }
        ImGui::PopStyleVar();

        
        if (!Get().mMatchedResults.empty()) 
        {
		    float height=ImGui::GetIO().Fonts->Fonts[0]->FontSize+2.0f+(2*ImGui::GetStyle().FramePadding.y);
	    	float winSize=std::min(10,(int)Get().mMatchedResults.size())*height;

	    	bool isShowingRecentMatches=Get().mMatchedResults.size()==Get().mRecentlyOpenedMatchedResults.size() && strlen(Get().mSearchBuffer)==0;
	    	if(isShowingRecentMatches)
	    		winSize+=ImGui::GetIO().Fonts->Fonts[0]->FontSize+ImGui::GetStyle().FramePadding.y*2;

	    	ImGui::PushStyleColor(ImGuiCol_ChildBg,IM_COL32(40, 40, 40, 255));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0.0f));
        	if(ImGui::BeginChild("##match_list", {-1.0f,winSize}, true))
        	{
        		if(isShowingRecentMatches)
        			ImGui::Text("Recent Results:");


				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(0,5.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,0.0f));

				bool isAnyItemSelected=false;

	        	for (int i = 0; i < Get().mMatchedResults.size(); ++i) 
	        	{
	            	bool isSelected = (i == selectedIndex);
	                if(FileNavigation::CustomSelectable(Get().mMatchedResults[i].filename,isSelected)){
	                	isAnyItemSelected=true;
	                	selectedIndex=i;
	                }
	            }

	            ImGui::PopStyleVar(2);

	            if(ImGui::IsKeyPressed(ImGuiKey_Enter) || isAnyItemSelected)
	            {
	            	std::string path=Get().mRootDirectory+"/"+Get().mMatchedResults[selectedIndex].filename;
	            	GL_INFO("Selected:{}",path);

	            	//Pushing the currently opened match to recentmatch vector if not present
	            	auto it=std::find_if(Get().mRecentlyOpenedMatchedResults.begin(),Get().mRecentlyOpenedMatchedResults.end(),[&](const MatchResult& a){
	            		return a.filename==Get().mMatchedResults[selectedIndex].filename;
	            	});
	            	if(it==Get().mRecentlyOpenedMatchedResults.end())
	            		Get().mRecentlyOpenedMatchedResults.push_back(Get().mMatchedResults[selectedIndex]);


	            	TabsManager::OpenFile(path);
	            	CloseQuickSearch();
	            	ImGui::CloseCurrentPopup();
	            }


			    //Scrolling with the selectedIndex
			    if(ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			    {
				    float scrollY=ImGui::GetScrollY();
				    int minIdx=scrollY/height;
				    int maxIdx=(scrollY+winSize)/height;

				    if(ImGui::IsKeyPressed(ImGuiKey_UpArrow))
				    {
						if(selectedIndex==0)
							selectedIndex=Get().mMatchedResults.size()-1;
						else
							selectedIndex--;
				    }
				    else if(ImGui::IsKeyPressed(ImGuiKey_DownArrow))
				    {
						if(selectedIndex==Get().mMatchedResults.size()-1)
							selectedIndex=0;
						else
							selectedIndex++;
				    }

				    if (selectedIndex <= minIdx) 
				        ImGui::SetScrollY(selectedIndex * height);
				    else if (selectedIndex >= maxIdx)
				        ImGui::SetScrollY((selectedIndex + 1) * height - winSize);
			    }


        	}
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::EndChild();
        }


        if((ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowedMaskForIsWindowHovered)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
        	ImGui::CloseCurrentPopup();
        	CloseQuickSearch();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
}

void QuickFileSearch::CloseQuickSearch()
{
	memset(Get().mSearchBuffer, 0, strlen(Get().mSearchBuffer));
	Get().mSelectedIndex=0;
	Get().mIsOpen=false;
}

void QuickFileSearch::EventListener(){
	if (ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_P)) {
        if(Get().mFiles.empty())
        {
        	Get().mRootDirectory=FileNavigation::GetFolders()[0];
        	Get().mFiles=GetFilesInDirectory(Get().mRootDirectory,Get().mRootDirectory,3);
        	Get().mMatchedResults.reserve(Get().mFiles.size());

        	for (const auto& filename : Get().mFiles) 
		    {
		        Get().mMatchedResults.push_back({ filename, 0 });
		    }
        }

        //Pushing recent files to mMatchedResults
        if(!Get().mRecentlyOpenedMatchedResults.empty())
        {
        	Get().mMatchedResults.clear();
        	for(auto& match:Get().mRecentlyOpenedMatchedResults)
        		Get().mMatchedResults.push_back(match);
        }

        Get().mIsOpen=true;
        GL_INFO(Get().mFiles.size());
    }
}

double QuickFileSearch::CalculateFuzzyScore(const std::string& aFileName, const std::string& aQuery) {
    int qLen = aQuery.size();
    int fLen = aFileName.size();
    
    if (qLen > fLen) return 0.0;  // query longer than aFileName → No match

    double score = 0.0;
    int qIdx = 0;

    for (int fIdx = 0; fIdx < fLen; ++fIdx) {
        if (qIdx < qLen && tolower(aQuery[qIdx]) == tolower(aFileName[fIdx])) {
            double matchScore = 10.0;

            if (fIdx == 0 || aFileName[fIdx - 1] == '/' || aFileName[fIdx - 1] == '_' || aFileName[fIdx - 1] == '-') {
                matchScore += 20.0;
            }

            if (qIdx > 0 && fIdx > 0 && tolower(aQuery[qIdx - 1]) == tolower(aFileName[fIdx - 1])) {
                matchScore += 5.0;
            }

            if (fIdx < 5) {
                matchScore += 10.0;
            }

            score += matchScore;
            qIdx++;  // move to next character in aQuery

            if (qIdx == qLen) break;
        }
    }

    return (qIdx == qLen) ? score : 0.0;
}