#include "WBGraphicsTextItemUndoCommand.h"
#include "WBGraphicsTextItem.h"

#include "core/memcheck.h"

WBGraphicsTextItemUndoCommand::WBGraphicsTextItemUndoCommand(WBGraphicsTextItem *textItem)
    : mTextItem(textItem)
{
    // NOOP
}

WBGraphicsTextItemUndoCommand::~WBGraphicsTextItemUndoCommand()
{
    // NOOP
}

void WBGraphicsTextItemUndoCommand::undo()
{
    if(mTextItem && mTextItem->document())
        mTextItem->document()->undo();
}

void WBGraphicsTextItemUndoCommand::redo()
{
    if(mTextItem && mTextItem->document())
        mTextItem->document()->redo();
}
