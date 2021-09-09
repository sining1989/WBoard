#ifndef WBTEXTTOOLS_H
#define WBTEXTTOOLS_H

#include <QString>

class WBTextTools{
public:
    WBTextTools(){}
    virtual ~WBTextTools(){}

    static QString cleanHtmlCData(const QString& _html);
    static QString cleanHtml(const QString& _html);
};

#endif // WBTEXTTOOLS_H
