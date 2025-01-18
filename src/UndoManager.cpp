#pragma once
#include "pch.h"
#include "Coordinates.h"
#include "UndoManager.h"
#include "TextEditor.h"
#include "TabsManager.h"

UndoRecord::UndoRecord(const std::vector<UndoOperation>& aOperations,
	EditorState& aBefore, EditorState& aAfter)
{
	mOperations = aOperations;
	mBefore = aBefore;
	mAfter = aAfter;
	for (const UndoOperation& o : mOperations)
		assert(o.mStart <= o.mEnd);
}

void UndoRecord::Undo(Editor* aEditor)
{
	for (int i = mOperations.size() - 1; i > -1; i--)
	{
		const UndoOperation& operation = mOperations[i];
		if (!operation.mText.empty())
		{
			switch (operation.mType)
			{
			case UndoOperationType::Delete:
			{
				auto start = operation.mStart;
				aEditor->InsertTextAt(start, operation.mText.c_str());
				aEditor->UpdateSyntaxHighlighting(operation.mStart.mLine - 1, operation.mEnd.mLine - operation.mStart.mLine + 2);
				break;
			}
			case UndoOperationType::Add:
			{
				aEditor->DeleteRange(operation.mStart, operation.mEnd);
				aEditor->UpdateSyntaxHighlighting(operation.mStart.mLine - 1, operation.mEnd.mLine - operation.mStart.mLine + 2);
				break;
			}
			}
		}
	}

	aEditor->mState = mBefore;
	aEditor->EnsureCursorVisible();
	aEditor->SetIsBufferModified(true);
}

void UndoRecord::Redo(Editor* aEditor)
{
	for (int i = 0; i < mOperations.size(); i++)
	{
		const UndoOperation& operation = mOperations[i];
		if (!operation.mText.empty())
		{
			switch (operation.mType)
			{
			case UndoOperationType::Delete:
			{
				aEditor->DeleteRange(operation.mStart, operation.mEnd);
				aEditor->UpdateSyntaxHighlighting(operation.mStart.mLine - 1, operation.mEnd.mLine - operation.mStart.mLine + 1);
				break;
			}
			case UndoOperationType::Add:
			{
				auto start = operation.mStart;
				aEditor->InsertTextAt(start, operation.mText.c_str());
				aEditor->UpdateSyntaxHighlighting(operation.mStart.mLine - 1, operation.mEnd.mLine - operation.mStart.mLine + 1);
				break;
			}
			}
		}
	}

	aEditor->mState = mAfter;
	aEditor->EnsureCursorVisible();
	aEditor->SetIsBufferModified(true);
}



void UndoManager::Undo(int aSteps,Editor* editor)
{
	while(CanUndo() && aSteps-->0){
		mUndoBuffer[--mUndoIndex].Undo(editor);
	}
}

void UndoManager::Redo(int aSteps,Editor* editor)
{
	while(CanRedo() && aSteps-->0){
		mUndoBuffer[mUndoIndex++].Redo(editor);
	}
}


void UndoManager::AddUndo(UndoRecord& aValue)
{
	mUndoBuffer.resize((size_t)(mUndoIndex+1));
	mUndoBuffer.back() = aValue;
	++mUndoIndex;
	TabsManager::GetCurrentActiveTextEditor()->SetIsBufferModified(true);
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

            for(int j=0;j<record.mOperations.size();j++){
            	const UndoOperation& uOperation=record.mOperations[j];

	            bool isAddOperation=uOperation.mType==UndoOperationType::Add;
	            if (i == mUndoIndex-1)
	            {
	                // Push the color for the background
	                ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.0f, 0.5f, 0.5f, 1.0f)); // Light green color
	                ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.0f, 0.5f, 0.5f, 1.0f)); // Light green for alternate rows
	            }


	            ImGui::TableNextRow();

	            //Type
	            ImGui::TableSetColumnIndex(0);
	            if(isAddOperation) ImGui::TextColored(ImVec4(41, 204, 155, 255),ICON_FA_PLUS);
	            else ImGui::TextColored(ImVec4(204, 41, 90, 255),ICON_FA_MINUS);

	            //Start
	            ImGui::TableSetColumnIndex(1);
	            ImGui::Text("(%d,%d)",uOperation.mStart.mLine,uOperation.mStart.mColumn);

	            //End
	            ImGui::TableSetColumnIndex(2);
	            ImGui::Text("(%d,%d)",uOperation.mEnd.mLine,uOperation.mEnd.mColumn);

	            //Content
	            ImGui::TableSetColumnIndex(3);
	            const auto& text=uOperation.mText;
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

        }

        ImGui::EndTable();
    }
}