#pragma once
#include "Coordinates.h"
#include "DataTypes.h"
#include "string"
#include "vector"

class Editor; //Forward Declaration of Editor Class

enum class UndoOperationType { Add, Delete };


struct UndoOperation
{
    std::string mText;
    Coordinates mStart;
    Coordinates mEnd;
    UndoOperationType mType;
};

class UndoRecord
{
public:
    UndoRecord() {}
    ~UndoRecord() {}

    UndoRecord(
        const std::vector<UndoOperation>& aOperations,
        EditorState& aBefore,
        EditorState& aAfter);

    void Undo(Editor* aEditor);
    void Redo(Editor* aEditor);

    std::vector<UndoOperation> mOperations;

    EditorState mBefore;
    EditorState mAfter;
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
    int GetUndoIndex()const{return mUndoIndex;}

    void DisplayUndoStack();

};
