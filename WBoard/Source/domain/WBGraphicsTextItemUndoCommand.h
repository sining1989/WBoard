#ifndef WBGRAPHICSTEXTITEMUNDOCOMMAND_H_
#define WBGRAPHICSTEXTITEMUNDOCOMMAND_H_

#include <QtWidgets>
#include "WBUndoCommand.h"

#include "WBGraphicsTextItem.h"


class WBGraphicsTextItemUndoCommand : public WBUndoCommand
{
    public:
        WBGraphicsTextItemUndoCommand(WBGraphicsTextItem *textItem);
        virtual ~WBGraphicsTextItemUndoCommand();

        virtual int getType() const { return WBUndoType::undotype_GRAPHICTEXTITEM; };

    protected:
        virtual void undo();
        virtual void redo();

    private:
        QPointer<WBGraphicsTextItem> mTextItem;
};

#endif /* WBGRAPHICSTEXTITEMUNDOCOMMAND_H_ */
