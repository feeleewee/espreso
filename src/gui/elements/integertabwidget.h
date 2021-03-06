#ifndef INTEGERTABWIDGET_H
#define INTEGERTABWIDGET_H

#include <QWidget>
#include <QTabWidget>

#include <memory>

#include "../../config/configuration.h"
#include "../../config/ecf/root.h"

#include "ecfobjectwidgetfactory.h"
#include "isavableobject.h"
#include "ivalidatableobject.h"

namespace espreso
{

class IntegerTabWidget : public QWidget, public ISavableObject, public IValidatableObject
{
    Q_OBJECT

public:
    IntegerTabWidget(ECFObject* map, std::unique_ptr<ECFObjectWidgetFactory> factory, QWidget* parent = 0);

    virtual void save() override;
    virtual bool isValid() override;
    virtual QString errorMessage() override;

private slots:
    void onTabClosed(int index);
    void onAddPressed();

private:
    QTabWidget* m_tabwidget;

    ECFObject* m_map;
    std::unique_ptr<ECFObjectWidgetFactory> m_factory;

    QString m_errmsg;

    int m_key;

    void addParam(ECFParameter*);
};

}

#endif // INTEGERTABWIDGET_H
