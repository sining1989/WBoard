#include "WBUndoCommand.h"

#include "core/memcheck.h"

WBUndoCommand::WBUndoCommand(QUndoCommand* parent):QUndoCommand(parent)
{
    // NOOP
}

WBUndoCommand::~WBUndoCommand()
{
    // NOOP
}

