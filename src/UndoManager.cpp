#pragma once
#include "FontAwesome6.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "pch.h"
#include "Coordinates.h"
#include "UndoManager.h"
#include "TextEditor.h"

UndoRecord::UndoRecord(
	const std::string& aAdded,
	const Coordinates aAddedStart,
	const Coordinates aAddedEnd,
	const std::string& aRemoved,
	const Coordinates aRemovedStart,
	const Coordinates aRemovedEnd,
	EditorState& aBefore,
	EditorState& aAfter)
	: mAddedText(aAdded)
	, mAddedStart(aAddedStart)
	, mAddedEnd(aAddedEnd)
	, mRemovedText(aRemoved)
	, mRemovedStart(aRemovedStart)
	, mRemovedEnd(aRemovedEnd)
	, mBeforeState(aBefore)
	, mAfterState(aAfter)
{
	assert(mAddedStart <= mAddedEnd);
	assert(mRemovedStart <= mRemovedEnd);
}


void UndoRecord::Undo(Editor* aEditor)
{
    // aEditor->DeleteRange(mAddedStart, mAddedEnd);  // Delete the added text
    // aEditor->InsertTextAt(mRemovedStart, mRemovedText.c_str());  // Insert back the removed text
    // aEditor->mState = mBeforeState;  // Restore the editor state
}

void UndoRecord::Redo(Editor* aEditor)
{
    // aEditor->DeleteRange(mRemovedStart, mRemovedEnd);  // Delete the removed text
    // aEditor->InsertTextAt(mAddedStart, mAddedText.c_str());  // Re-insert the added text
    // aEditor->mState = mAfterState;  // Restore the editor state
}



void UndoManager::Undo(int aSteps,Editor* editor)
{
	if(!CanUndo()) return;
	GL_INFO("IDX:{}, SIZE:{}",mUndoIndex,mUndoBuffer.size());
	if(!mUndoBuffer[mUndoIndex-1].isBatchEnd){
		while (CanUndo() && aSteps-- > 0){
			if(mUndoBuffer[mUndoIndex-1].isBatchEnd) break;
			else mUndoBuffer[--mUndoIndex].Undo(editor);
		}
	}else{
		--mUndoIndex;
		while(CanUndo()){
			mUndoBuffer[mUndoIndex].Undo(editor);
			if(mUndoBuffer[mUndoIndex--].isBatchStart) break;
		}
		++mUndoIndex;
	}
}

void UndoManager::Redo(int aSteps,Editor* editor)
{
	if(!CanRedo()) return;
	// while (CanRedo() && aSteps-- > 0) mUndoBuffer[mUndoIndex++].Redo(editor);
	if(!mUndoBuffer[mUndoIndex].isBatchStart){
		while(CanRedo() && aSteps-->0){
			if(mUndoBuffer[mUndoIndex].isBatchStart) break;
			else mUndoBuffer[mUndoIndex++].Redo(editor);
		}
	}else{
		while(CanRedo()){
			mUndoBuffer[mUndoIndex].Redo(editor);
			if(mUndoBuffer[mUndoIndex++].isBatchEnd) break;
		}
	}
}


void UndoManager::AddUndo(UndoRecord& aValue)
{
	mUndoBuffer.resize((size_t)(mUndoIndex+1));
	mUndoBuffer.back() = aValue;
	++mUndoIndex;
}

void UndoManager::DisplayUndoStack(){
    if (ImGui::BeginTable("Undo Records", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        // Table headers
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Start");
        ImGui::TableSetupColumn("End");
        ImGui::TableSetupColumn("Content");
        ImGui::TableHeadersRow();

        // Iterate through mUndoBuffer and display data
        for (int i = mUndoBuffer.size()-1; i >=0; --i)
        {
            const UndoRecord& record = mUndoBuffer[i];
            bool isAddedText=!record.mAddedText.empty();
            if (i == mUndoIndex-1)
            {
                // Push the color for the background
                ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.0f, 0.5f, 0.5f, 1.0f)); // Light green color
                ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.0f, 0.5f, 0.5f, 1.0f)); // Light green for alternate rows
            }


            ImGui::TableNextRow();

            //Type
            ImGui::TableSetColumnIndex(0);
            if(isAddedText) ImGui::TextColored(ImVec4(41, 204, 155, 255),ICON_FA_PLUS);
            else ImGui::TextColored(ImVec4(204, 41, 90, 255),ICON_FA_MINUS);

            //Start
            ImGui::TableSetColumnIndex(1);
            if(isAddedText) ImGui::Text("(%d,%d)",record.mAddedStart.mLine,record.mAddedStart.mColumn);
            else ImGui::Text("(%d,%d)",record.mRemovedStart.mLine,record.mRemovedStart.mColumn);

            //End
            ImGui::TableSetColumnIndex(2);
            if(isAddedText) ImGui::Text("(%d,%d)",record.mAddedEnd.mLine,record.mAddedEnd.mColumn);
            else ImGui::Text("(%d,%d)",record.mRemovedEnd.mLine,record.mRemovedEnd.mColumn);

            //Content
            ImGui::TableSetColumnIndex(3);
            const auto& text=isAddedText ? record.mAddedText : record.mRemovedText;
            if(text.size()>10){
            	ImGui::Text("View");
	            if (ImGui::IsItemHovered()){
	                ImGui::BeginTooltip();
	                ImGui::Text("%s", text.c_str());
	                ImGui::EndTooltip();
	            }
            }else{
            	char x=text[0];
            	if(x=='\n') ImGui::Text("\\n");
            	else if(x=='\t') ImGui::Text("\\t");
            	else if(x==' ') ImGui::Text("Space");
            	else ImGui::Text("%s", text.c_str());
            }

            if (i == mUndoIndex-1) ImGui::PopStyleColor(2);

        }

        ImGui::EndTable();
    }
}