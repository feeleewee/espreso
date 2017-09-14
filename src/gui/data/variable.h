#ifndef VARIABLE_H
#define VARIABLE_H

#include <QString>
#include "datatype.h"
#include "namedentity.h"
//#include <QMetaType>

class Variable : public NamedEntity
{
private:
    DataType* mData;

public:
    Variable();
    Variable(const QString& name, DataType* data);
    Variable(const Variable&);
    ~Variable();

    DataType* data() const;
    QString toString() const;
};

//Q_DECLARE_METATYPE(Variable);

#endif // VARIABLE_H
