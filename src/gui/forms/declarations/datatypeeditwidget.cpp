#include "datatypeeditwidget.h"
#include "ui_datatypeeditwidget.h"

#include "../validators/validatordelegate.h"
#include "../elements/expressionedit.h"
#include <QPair>

DataTypeEditWidget::DataTypeEditWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataTypeEditWidget)
{
    ui->setupUi(this);

    this->createUi();

    this->activeType = 0;
    this->uiExpression->show();
}

DataTypeEditWidget::DataTypeEditWidget(const DataType *data, QWidget *parent) :
    DataTypeEditWidget(parent)
{
    this->uiExpression->hide();\

    if (const ExpressionType* t = dynamic_cast<const ExpressionType*>(data))
    {
        this->initExpression(t);
        this->activeType = 0;
    }
    else if (const TableType* t = dynamic_cast<const TableType*>(data))
    {
        this->initTable(t);
        this->activeType = 1;
    }
    else if (const PiecewiseFunctionType* t =
             dynamic_cast<const PiecewiseFunctionType*>(data))
    {
        this->initPiecewise(t);
        this->activeType = 2;
    }
    else
    {
        qWarning("%s", tr("DataTypeEdit: Unknown DataType subclass!").toStdString().c_str());
    }
}

DataTypeEditWidget::~DataTypeEditWidget()
{
    delete ui;
}

void DataTypeEditWidget::createUi()
{
    ExpressionEdit* function = new ExpressionEdit(this);
    connect(function, &ExpressionEdit::validStateChanged, this,
            &DataTypeEditWidget::changeValidState);
    function->hide();

    TableTypeWidget* table = new TableTypeWidget(this);
    table->hide();

    PiecewiseTypeWidget* piecewise = new PiecewiseTypeWidget(this);
    connect(piecewise, &PiecewiseTypeWidget::validStateChanged, this,
            &DataTypeEditWidget::changeValidState);
    piecewise->hide();

    this->uiExpression = function;
    this->uiTable = table;
    this->uiPiecewise = piecewise;

    ui->layout->addWidget(uiExpression);
    ui->layout->addWidget(uiTable);
    ui->layout->addWidget(uiPiecewise);
}

void DataTypeEditWidget::initExpression(const ExpressionType* et)
{
    this->uiExpression->setText(et->toString());
    this->uiExpression->show();
}

void DataTypeEditWidget::initTable(const TableType* tt)
{
    this->uiTable->addData(tt->data());
    this->uiTable->show();
}

void DataTypeEditWidget::initPiecewise(const PiecewiseFunctionType* pft)
{
    this->uiPiecewise->addData(pft->data());
    this->uiPiecewise->show();
}

QComboBox* DataTypeEditWidget::createComboBox(QWidget *parent)
{
    QComboBox* box = new QComboBox(parent);
    box->addItems(DataTypeVisitor::types());
    box->setCurrentIndex(this->activeType);

    connect(box, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DataTypeEditWidget::changeType);

    return box;
}

void DataTypeEditWidget::changeType(int index)
{
    switch (index)
    {
        case 0:
            this->uiTable->hide();
            this->uiPiecewise->hide();
            this->uiExpression->show();
            break;
        case 1:
            this->uiExpression->hide();
            this->uiPiecewise->hide();
            this->uiTable->show();
            break;
        case 2:
            this->uiExpression->hide();
            this->uiTable->hide();
            this->uiPiecewise->show();
            break;
        default:
            qWarning("%s: %d!",
                     tr("DataTypeEditWidget: Unknown DataType ID").toStdString().c_str(),
                     index);
            return;
    }

    this->activeType = index;
}

void DataTypeEditWidget::changeValidState(bool valid)
{
    emit validStateChanged(valid);
}
