#include "physicswidget.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QLabel>
#include <QDebug>

#include "../validators/validatorfactory.h"
#include "../elements/spinnerhandler.h"

using namespace espreso;

PhysicsWidget::PhysicsWidget(ECFConfiguration* ecf, Mesh* mesh, QWidget* parent) :
    ECFObjectWidget(ecf, parent)
{
    this->m_ecf = ecf;
    this->m_mesh = mesh;
}

QWidget* PhysicsWidget::initContainer()
{
    ECFParameter* physics = m_ecf->getParameter("physics");

    QScrollArea* area = new QScrollArea;

    QWidget* widget = new QWidget(area);
    this->m_widget = widget;
    QVBoxLayout* w_layout = new QVBoxLayout;
    widget->setLayout(w_layout);

    QComboBox* cmbPhysics = new QComboBox(widget);
    w_layout->addWidget(cmbPhysics);

    int active = 0;
    int index = 0;
    for (auto option = physics->metadata.options.begin(); option != physics->metadata.options.end(); ++option, ++index)
    {
        QString name = QString::fromStdString(option->name);
        cmbPhysics->addItem(name);

        if (option->name.compare(physics->getValue()) == 0) active = index;
    }

    cmbPhysics->setCurrentIndex(active);
    this->m_physics = cmbPhysics;
    this->m_obj = this->physics(active);
    connect(cmbPhysics, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PhysicsWidget::onPhysicsChange);

    area->setWidgetResizable(true);
    area->setWidget(widget);

    return area;
}

ECFObject* PhysicsWidget::physics(int index)
{
    switch (index)
    {
        case 0:
            return &m_ecf->heat_transfer_2d;
        case 1:
            return &m_ecf->heat_transfer_3d;
        case 2:
            return &m_ecf->structural_mechanics_2d;
        case 3:
            return &m_ecf->structural_mechanics_3d;
        default:
            qCritical() << tr("WorkflowWidget: Invalid index of physics!");
            return nullptr;
    }
}

void PhysicsWidget::onPhysicsChange(int index)
{
    ECFParameter* physics = m_ecf->getParameter("physics");
    physics->setValue(physics->metadata.options[index].name);

    this->m_obj = this->physics(index);
    this->m_properties = nullptr;

    this->redraw();

    emit physicsChanged(this->m_obj);
}

void PhysicsWidget::drawObject(ECFObject* obj)
{
    if (obj->name.compare("material_set") == 0 || obj->name.compare("load_steps_settings") == 0)
        return;

    if ( obj->metadata.datatype.size() == 2 )
    {
        if (this->m_properties == nullptr)
        {
            this->m_properties = new RegionPropertyWidget(m_mesh,
                                                          static_cast<PhysicsConfiguration*>(this->activePhysics()),
                                                          this->m_container,
                                                          tr("Region properties"));
        }
        this->m_properties->addProperty(obj);
        this->m_widget->layout()->addWidget(m_properties);

        return;
    }

    QWidget* widget = new QWidget(this->m_container);
    QLayout* layout;
    if (obj->parameters.size()) layout = new QFormLayout;
    else layout = new QVBoxLayout;
    widget->setLayout(layout);

    QSpacerItem* verticalSpacer = new QSpacerItem(0,
                                                  0,
                                                  QSizePolicy::Minimum,
                                                  QSizePolicy::Expanding);
    layout->addItem(verticalSpacer);

    this->m_widget->layout()->addWidget(widget);

    this->createHeadline(obj, widget);

    this->processParameters(obj, widget);
}

FormWidget* PhysicsWidget::processPositiveInteger(ECFParameter* parameter, FormWidget* form, QWidget* widget)
{
    FormWidget* fw = this->createFormWidget(widget, form);
    if (parameter->name.compare("load_steps") == 0)
    {
        SpinnerHandler* handler = new SpinnerHandler(parameter, false, widget);
        connect(handler, &SpinnerHandler::valueChanged, this, &PhysicsWidget::onLoadstepsChange);
        fw->appendRow(QString::fromStdString(parameter->metadata.description[0]),
                handler);
        this->m_savables.append(handler);
        this->m_validatables.append(handler);
    }
    else
    {
        return ECFObjectWidget::processPositiveInteger(parameter, form, widget);
    }
}

void PhysicsWidget::onLoadstepsChange(int loadsteps)
{
    emit this->loadstepsChanged(loadsteps);
}

ECFObject* PhysicsWidget::activePhysics()
{
    return this->physics(m_physics->currentIndex());
}
