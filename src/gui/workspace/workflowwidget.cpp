#include "workflowwidget.h"
#include "ui_workflowwidget.h"

#include <QFileDialog>
#include <QLabel>
#include <QComboBox>
#include <QDebug>
#include <QScrollArea>

#include "loadstepwidget.h"
#include "regionmaterialswidget.h"
#include "outputconfigurationwidget.h"

using namespace espreso;

WorkflowWidget::WorkflowWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WorkflowWidget)
{
    ui->setupUi(this);
}

WorkflowWidget::~WorkflowWidget()
{
    delete ui;
}

void WorkflowWidget::on_btnMesh_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open a file"),
                                                    "", tr("Espreso configuration file (*.ecf)"));
    if (filename.size() == 0)
        return;

    emit fileOpened(filename);
}

void WorkflowWidget::setData(ECFConfiguration *ecf, Mesh* mesh)
{   
    int tabs = ui->workflow->count();
    for (int i = 1; i < tabs; i++)
    {
        ui->workflow->removeTab(1);
    }

    this->m_ecf = ecf;
    this->m_physicsTab = nullptr;
    this->m_mesh = mesh;

    this->createInput();

    this->createPhysicsTab();

    this->createMaterialsTab();

    this->createLoadstepsTabs();

    this->m_loadsteps_fst_tab_index = ui->workflow->count();

    this->createOutputTab();
}

void WorkflowWidget::createInput()
{
    ECFParameter* i = this->m_ecf->getParameter("input");
    if (!this->m_inputBox_filled)
    {
        for (auto opt = i->metadata.options.begin(); opt != i->metadata.options.end(); ++opt)
        {
            ui->cmbInput->addItem( QString::fromStdString((*opt).description) );
        }
        connect(ui->cmbInput, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &WorkflowWidget::onInputChange);
        this->m_inputBox_filled = true;
    }
    int index = 0;
    for (auto opt = i->metadata.options.begin(); opt != i->metadata.options.end(); ++opt)
    {
        if ((*opt).name.compare(i->getValue()) == 0) break;
        index++;
    }
    ui->cmbInput->setCurrentIndex(index);

    this->onInputChange(ui->cmbInput->currentIndex());
}

void WorkflowWidget::onInputChange(int)
{
    if (this->m_inputWidget != nullptr)
    {
        this->m_inputWidget->hide();
        ui->inputConfigLayout->removeWidget(this->m_inputWidget);
    }

    this->m_inputWidget = new FixedECFObjectWidget(this->input());
    this->m_inputWidget->init();
    ui->inputConfigLayout->addWidget(this->m_inputWidget);
}

ECFObject* WorkflowWidget::input()
{
    switch (ui->cmbInput->currentIndex())
    {
    case 0:
        return &this->m_ecf->workbench;
    case 1:
        return &this->m_ecf->openfoam;
    case 2:
        return &this->m_ecf->esdata;
    case 3:
        return &this->m_ecf->generator;
    default:
        return &this->m_ecf->generator;
    }
}

void WorkflowWidget::createPhysicsTab()
{
    PhysicsWidget* pw = new PhysicsWidget(this->m_ecf, this->m_mesh, this);
    pw->init();

    this->m_phyDetail = pw;
    this->m_loadsteps = QString::fromStdString(
                pw->activePhysics()
                        ->getParameter("load_steps")
                        ->getValue()
                ).toInt();
    connect(pw, &PhysicsWidget::loadstepsChanged, this, &WorkflowWidget::onLoadstepsChange);
    connect(pw, &PhysicsWidget::physicsChanged, this, &WorkflowWidget::onPhysicsChange);

    ui->workflow->addTab(pw, QLatin1String("Physics"));
    ui->workflow->setCurrentIndex(ui->workflow->count() - 1);
}

void WorkflowWidget::createMaterialsTab()
{
    RegionMaterialsWidget* rmw = new RegionMaterialsWidget(this->m_mesh,
                                                           this->activePhysics(this->m_ecf),
                                                           this);
    ui->workflow->addTab(rmw, QLatin1String("Materials"));
}

void WorkflowWidget::createOutputTab()
{
    OutputConfigurationWidget* ocw = new OutputConfigurationWidget(&this->m_ecf->output, this);
    ocw->init();
    ui->workflow->addTab(ocw, QLatin1String("Output"));
}

void WorkflowWidget::createLoadstepsTabs()
{
    for (int i = 0; i < m_loadsteps; i++)
    {
        LoadstepWidget* lsw = new LoadstepWidget(i + 1, this->m_mesh, m_phyDetail->activePhysics(), this);
        lsw->init();
        ui->workflow->addTab(lsw, tr("Loadstep %1").arg(i + 1));
    }
}

void WorkflowWidget::onLoadstepsChange(int loadsteps)
{
    int delta = this->m_loadsteps - loadsteps;
    if (delta > 0)
    {
        //DELETE LOADSTEPS

        ui->workflow->removeTab(this->m_loadsteps + this->m_loadsteps_fst_tab_index - 2);

        this->m_loadsteps--;
    }
    else if (delta < 0)
    {
        //ADD LOADSTEPS

        LoadstepWidget* lsw = new LoadstepWidget(++this->m_loadsteps, this->m_mesh, m_phyDetail->activePhysics(), this);
        lsw->init();
        ui->workflow->addTab(lsw, tr("Loadstep %1").arg(this->m_loadsteps));
    }
}

PhysicsConfiguration* WorkflowWidget::activePhysics(ECFConfiguration* ecf)
{
    switch (ecf->physics)
    {
    case PHYSICS::HEAT_TRANSFER_2D:
        return &ecf->heat_transfer_2d;
    case PHYSICS::HEAT_TRANSFER_3D:
        return &ecf->heat_transfer_3d;
    case PHYSICS::STRUCTURAL_MECHANICS_2D:
        return &ecf->structural_mechanics_2d;
    case PHYSICS::STRUCTURAL_MECHANICS_3D:
        return &ecf->structural_mechanics_3d;
    default:
        qFatal("WorkflowWidget: Unknown physics!");
        return nullptr;
    }
}

void WorkflowWidget::onPhysicsChange(ECFObject *physics)
{

    int tabs = ui->workflow->count();
    for (int i = 2; i < tabs; i++)
    {
        ui->workflow->removeTab(2);
    }

    this->createMaterialsTab();

    this->m_loadsteps = QString::fromStdString(
                this->m_phyDetail
                        ->activePhysics()
                            ->getParameter("load_steps")
                                ->getValue()
                ).toInt();

    this->createLoadstepsTabs();

    this->m_loadsteps_fst_tab_index = ui->workflow->count();

    this->createOutputTab();

    emit physicsChanged(physics);
}
