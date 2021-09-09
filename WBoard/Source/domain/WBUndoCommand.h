#ifndef WBABSTRACTUNDOCOMMAND_H_
#define WBABSTRACTUNDOCOMMAND_H_

#include <QtWidgets>
#include <QUndoCommand>
#include <core/WB.h>

class WBUndoCommand : public QUndoCommand
{
public:
    WBUndoCommand(QUndoCommand *parent = 0);
    ~WBUndoCommand();

    virtual int getType() const { return WBUndoType::undotype_UNKNOWN; }

};

#endif /* WBABSTRACTUNDOCOMMAND_H_ */
