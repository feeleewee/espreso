#include "declarationswidget.h"
#include "ui_declarationswidget.h"

#include "../models/treemodel.h"
#include "declarations/variabledialog.h"
#include "../data/variable.h"

#include <QStandardItemModel>

DeclarationsWidget::DeclarationsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeclarationsWidget)
{
    ui->setupUi(this);

//    ui->DeclarationTree->setModel(new TreeModel(tr("Declarations")));
    this->setupTree();
    this->createActions();
}

DeclarationsWidget::~DeclarationsWidget()
{
    delete ui;
}

void DeclarationsWidget::setupTree()
{
    this->treeModel = new QStandardItemModel();
    QStandardItem* parent = this->treeModel->invisibleRootItem();
    treeVars = new QStandardItem("Variables");
    QStandardItem* cs = new QStandardItem("Coordinate Systems");
    QStandardItem* mats = new QStandardItem("Materials");
    parent->appendRow(treeVars);
    parent->appendRow(cs);
    parent->appendRow(mats);
    ui->DeclarationTree->setModel(this->treeModel);
    ui->DeclarationTree->setEditTriggers(QAbstractItemView::NoEditTriggers);

//    QAbstractItemModel* model = ui->DeclarationTree->model();
//    model->insertRows(0, 3);
//    QVariant vars(tr("Variables"));
//    QVariant csystems(tr("Coordinate Systems"));
//    QVariant mats(tr("Materials"));
//    QModelIndex varIndex = model->index(0, 0);
//    QModelIndex csIndex = model->index(1, 0);
//    QModelIndex matsIndex = model->index(2, 0);
//    model->setData(varIndex, vars);
//    model->setData(csIndex, csystems);
//    model->setData(matsIndex, mats);
}


void DeclarationsWidget::on_DeclarationTree_customContextMenuRequested(const QPoint &pos)
{
    QMenu treeMenu(this);
    treeMenu.addAction(this->newItem);
    treeMenu.exec(ui->DeclarationTree->mapToGlobal(pos));
}

void DeclarationsWidget::createActions()
{
    this->newItem = new QAction(tr("&New"), this);
    connect(this->newItem, &QAction::triggered, this, &DeclarationsWidget::treeNewItem);

    this->editItem = new QAction(tr("&Edit"), this);
    connect(this->editItem, &QAction::triggered, this, &DeclarationsWidget::treeEditItem);

    this->delItem = new QAction(tr("&Delete"), this);
    connect(this->delItem, &QAction::triggered, this, &DeclarationsWidget::treeDelItem);
}

void DeclarationsWidget::treeNewItem()
{
    QModelIndexList indexList = ui->DeclarationTree->selectionModel()->selectedIndexes();
    if (!indexList.count())
        return;

    VariableDialog* dialog = new VariableDialog(this);
    if (dialog->exec() == QDialog::Accepted)
    {
        QStandardItem* item = new QStandardItem("Test");
        treeVars->appendRow(item);
    }

//    QVariant data = index.data();
//    if (data.canConvert<Variable>())
//    {
//        VariableDialog* dialog = new VariableDialog(this);
//        if (dialog->exec() == QDialog::Accepted)
//        {
//            QAbstractItemModel* model = ui->DeclarationTree->model();
//            int row = ++this->varCount;
//            QModelIndex parent = model->index(0, 0);
//            model->insertRows(row, 1, parent);
//            QModelIndex index = model->index(row, 0);
//            QVariant data = dialog->data();
//            model->setData(index, data);
//        }
//    }
}

void DeclarationsWidget::treeEditItem()
{

}

void DeclarationsWidget::treeDelItem()
{

}
