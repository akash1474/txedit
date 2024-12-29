#pragma once
#include "Coordinates.h"
#include "DataTypes.h"
#include "string"
#include "vector"

class Editor; //Forward Declaration of Editor Class

class UndoRecord
{
public:
    UndoRecord() {}
    ~UndoRecord() {}

    UndoRecord(
        const std::string& aAddedText,         // Text added
        const Coordinates aAddedStart,     // Start position of added text
        const Coordinates aAddedEnd,       // End position of added text
        const std::string& aRemovedText,       // Text removed
        const Coordinates aRemovedStart,   // Start position of removed text
        const Coordinates aRemovedEnd,     // End position of removed text
        EditorState& aBeforeState,      // State before change
        EditorState& aAfterState       // State after change
    );

    void Undo(Editor* aEditor);
    void Redo(Editor* aEditor);

    std::string mAddedText;
    Coordinates mAddedStart;
    Coordinates mAddedEnd;
    std::string mRemovedText;
    Coordinates mRemovedStart;
    Coordinates mRemovedEnd;
    EditorState mBeforeState;
    EditorState mAfterState;
    bool isBatchStart{0};
    bool isBatchEnd{0};
};



class UndoManager{
public:
    UndoManager():mUndoIndex(0){}

private:
    int mUndoIndex;
    std::vector<UndoRecord> mUndoBuffer;

    bool CanUndo()const{return mUndoIndex>0;};
    bool CanRedo()const{return mUndoIndex<(int)mUndoBuffer.size();}

public:
    void Undo(int aSteps,Editor* editor);
    void Redo(int aSteps,Editor* editor);

    void AddUndo(UndoRecord& aValue);

    void DisplayUndoStack();

};
